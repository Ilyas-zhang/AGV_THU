/*
 * line_follow.c — 四路循迹高速平滑版
 *，
 * 传感器：0 = 黑线，1 = 白地。
 * 当前物理顺序（左→右）：X2  X1  X3  X4。
 *
 * 控制核心：
 *   1) 只有三个运动状态：直行 / 左转 / 右转，尽量减少动作切换；
 *   2) 圆弧只做“轻反转内侧 + 高速前转外侧”，避免等速原地旋转；
 *   3) 外侧探头触发或转弯中持续丢线时，只升级一次到强直角转弯；
 *   4) 短暂回中、短暂反向噪声都不会马上改动作；
 *   5) 中心连续稳定后才恢复直行，保持“先转正，再直行”；
 *   6) 正反换向强制经过 0，且“制动快、反向起步慢”，兼顾速度与丝滑。
 */

#include "line_follow.h"
#include "line_follow_config.h"
#include "irtracking.h"
#include "motor.h"
#include "motor_config.h"

typedef enum {
    LF_TURN_LEFT  = -1,
    LF_STRAIGHT   = 0,
    LF_TURN_RIGHT = 1
} LineFollowMode;

typedef enum {
    LF_LEVEL_ARC    = 1,
    LF_LEVEL_CORNER = 2
} LineFollowTurnLevel;

static LineFollowMode mode = LF_STRAIGHT;
static LineFollowTurnLevel turn_level = LF_LEVEL_ARC;

static int8_t last_error = 0;
static int8_t last_turn = 0;              /* -1=物理左转, +1=物理右转 */

static uint8_t center_confirm = 0;
static uint8_t opposite_confirm = 0;
static uint8_t lost_count = 0;
static uint8_t turn_hold_count = 0;
static uint8_t turn_lost_count = 0;

/* 物理左右轮当前/目标有符号速度：正=前进，负=后退 */
static int16_t current_left = 0;
static int16_t current_right = 0;
static int16_t target_left = 0;
static int16_t target_right = 0;

static int16_t clamp_speed(int16_t value)
{
    if (value > MOTOR_MAX_SPEED) return MOTOR_MAX_SPEED;
    if (value < -MOTOR_MAX_SPEED) return -MOTOR_MAX_SPEED;
    return value;
}

static int16_t abs_speed(int16_t value)
{
    return (value < 0) ? (int16_t)(-value) : value;
}

static int16_t move_toward(int16_t current, int16_t target, int16_t step)
{
    if (current < target) {
        int32_t next = (int32_t)current + step;
        return (next > target) ? target : (int16_t)next;
    }
    if (current > target) {
        int32_t next = (int32_t)current - step;
        return (next < target) ? target : (int16_t)next;
    }
    return current;
}

/*
 * 平滑换向：
 *   - 符号相反：先用较大的 brake step 快速回到 0；
 *   - 到 0 后：进入反向时用较小 accel step，减少机械冲击；
 *   - 同方向：使用普通斜坡，保持高速响应。
 */
static int16_t slew_toward(int16_t current, int16_t target)
{
    if ((current > 0 && target < 0) || (current < 0 && target > 0)) {
        return move_toward(current, 0, LINE_FOLLOW_REVERSE_BRAKE_STEP);
    }

    if (current == 0 && target < 0) {
        return move_toward(current, target, LINE_FOLLOW_REVERSE_ACCEL_STEP);
    }

    return move_toward(current, target, LINE_FOLLOW_SLEW_STEP);
}

static int16_t pwm_limit(int16_t pwm)
{
    if (pwm < 0) return 0;
    if (pwm > MOTOR_MAX_SPEED) return MOTOR_MAX_SPEED;
    return pwm;
}

static void drive_motor1_signed(int16_t command)
{
    int16_t mag = pwm_limit(MOTOR1_PWM(abs_speed(command)));
    if (command > 0) pwm_motor1_forward(mag);
    else if (command < 0) pwm_motor1_backward(mag);
    else pwm_motor1_forward(0);
}

static void drive_motor2_signed(int16_t command)
{
    int16_t mag = pwm_limit(MOTOR2_PWM(abs_speed(command)));
    if (command > 0) pwm_motor2_forward(mag);
    else if (command < 0) pwm_motor2_backward(mag);
    else pwm_motor2_forward(0);
}

static void drive_motor3_signed(int16_t command)
{
    int16_t mag = pwm_limit(MOTOR3_PWM(abs_speed(command)));
    if (command > 0) pwm_motor3_forward(mag);
    else if (command < 0) pwm_motor3_backward(mag);
    else pwm_motor3_forward(0);
}

static void drive_motor4_signed(int16_t command)
{
    int16_t mag = pwm_limit(MOTOR4_PWM(abs_speed(command)));
    if (command > 0) pwm_motor4_forward(mag);
    else if (command < 0) pwm_motor4_backward(mag);
    else pwm_motor4_forward(0);
}

/* 将“物理左/右轮”映射到当前项目四个电机编号。 */
static void apply_physical_wheels(int16_t left, int16_t right)
{
#if LINE_FOLLOW_SWAP_MOTOR_SIDES
    drive_motor3_signed(left);
    drive_motor4_signed(left);
    drive_motor1_signed(right);
    drive_motor2_signed(right);
#else
    drive_motor1_signed(left);
    drive_motor2_signed(left);
    drive_motor3_signed(right);
    drive_motor4_signed(right);
#endif
}

static void set_target_wheels(int16_t left, int16_t right)
{
    target_left = clamp_speed(left);
    target_right = clamp_speed(right);
}

static void update_wheel_output(void)
{
    current_left = slew_toward(current_left, target_left);
    current_right = slew_toward(current_right, target_right);
    apply_physical_wheels(current_left, current_right);
}

static void command_straight(int16_t speed)
{
    speed = abs_speed(clamp_speed(speed));
    set_target_wheels(speed, speed);
}

static void command_turn_left_pair(int16_t inner_reverse, int16_t outer_forward)
{
    inner_reverse = abs_speed(clamp_speed(inner_reverse));
    outer_forward = abs_speed(clamp_speed(outer_forward));
    set_target_wheels((int16_t)-inner_reverse, outer_forward);
}

static void command_turn_right_pair(int16_t inner_reverse, int16_t outer_forward)
{
    inner_reverse = abs_speed(clamp_speed(inner_reverse));
    outer_forward = abs_speed(clamp_speed(outer_forward));
    set_target_wheels(outer_forward, (int16_t)-inner_reverse);
}

static void command_arc_turn(LineFollowMode direction)
{
    if (direction == LF_TURN_LEFT) {
        command_turn_left_pair(LINE_FOLLOW_ARC_INNER_REVERSE,
                               LINE_FOLLOW_ARC_OUTER_FORWARD);
    } else {
        command_turn_right_pair(LINE_FOLLOW_ARC_INNER_REVERSE,
                                LINE_FOLLOW_ARC_OUTER_FORWARD);
    }
}

static void command_corner_turn(LineFollowMode direction)
{
    if (direction == LF_TURN_LEFT) {
        command_turn_left_pair(LINE_FOLLOW_CORNER_INNER_REVERSE,
                               LINE_FOLLOW_CORNER_OUTER_FORWARD);
    } else {
        command_turn_right_pair(LINE_FOLLOW_CORNER_INNER_REVERSE,
                                LINE_FOLLOW_CORNER_OUTER_FORWARD);
    }
}

static void command_settle_turn(LineFollowMode direction)
{
    if (direction == LF_TURN_LEFT) {
        command_turn_left_pair(LINE_FOLLOW_SETTLE_INNER_REVERSE,
                               LINE_FOLLOW_SETTLE_OUTER_FORWARD);
    } else {
        command_turn_right_pair(LINE_FOLLOW_SETTLE_INNER_REVERSE,
                                LINE_FOLLOW_SETTLE_OUTER_FORWARD);
    }
}

static void command_search_turn(LineFollowMode direction)
{
    if (direction == LF_TURN_LEFT) {
        command_turn_left_pair(LINE_FOLLOW_SEARCH_INNER_REVERSE,
                               LINE_FOLLOW_SEARCH_OUTER_FORWARD);
    } else {
        command_turn_right_pair(LINE_FOLLOW_SEARCH_INNER_REVERSE,
                                LINE_FOLLOW_SEARCH_OUTER_FORWARD);
    }
}

static void command_current_turn(void)
{
    if (turn_level == LF_LEVEL_CORNER) {
        command_corner_turn(mode);
    } else {
        command_arc_turn(mode);
    }
}

/* ---- 原地差速转弯（已注释，保留回退） ----
 * 内外侧等速反转，纯旋转无平移。
 *
static void command_arc_turn(LineFollowMode direction)
{
    if (direction == LF_TURN_LEFT) {
        command_turn_left_pair(LINE_FOLLOW_ARC_SPIN_SPEED,
                               LINE_FOLLOW_ARC_SPIN_SPEED);
    } else {
        command_turn_right_pair(LINE_FOLLOW_ARC_SPIN_SPEED,
                                LINE_FOLLOW_ARC_SPIN_SPEED);
    }
}

static void command_corner_turn(LineFollowMode direction)
{
    if (direction == LF_TURN_LEFT) {
        command_turn_left_pair(LINE_FOLLOW_CORNER_SPIN_SPEED,
                               LINE_FOLLOW_CORNER_SPIN_SPEED);
    } else {
        command_turn_right_pair(LINE_FOLLOW_CORNER_SPIN_SPEED,
                                LINE_FOLLOW_CORNER_SPIN_SPEED);
    }
}

static void command_settle_turn(LineFollowMode direction)
{
    if (direction == LF_TURN_LEFT) {
        command_turn_left_pair(LINE_FOLLOW_SETTLE_SPIN_SPEED,
                               LINE_FOLLOW_SETTLE_SPIN_SPEED);
    } else {
        command_turn_right_pair(LINE_FOLLOW_SETTLE_SPIN_SPEED,
                                LINE_FOLLOW_SETTLE_SPIN_SPEED);
    }
}

static void command_search_turn(LineFollowMode direction)
{
    if (direction == LF_TURN_LEFT) {
        command_turn_left_pair(LINE_FOLLOW_SEARCH_SPIN_SPEED,
                               LINE_FOLLOW_SEARCH_SPIN_SPEED);
    } else {
        command_turn_right_pair(LINE_FOLLOW_SEARCH_SPIN_SPEED,
                                LINE_FOLLOW_SEARCH_SPIN_SPEED);
    }
}
*/

/*
 * 物理位置误差：
 * 左外=-3，左内=-1，右内=+1，右外=+3。
 * 多探头同时压线时取平均，只用于方向判断；转弯力度由具体外侧探头决定。
 */
static int8_t compute_line_position(uint8_t left_outer,
                                    uint8_t left_inner,
                                    uint8_t right_inner,
                                    uint8_t right_outer)
{
    int16_t sum = 0;
    int16_t count = 0;

    if (left_outer)  { sum -= 3; count++; }
    if (left_inner)  { sum -= 1; count++; }
    if (right_inner) { sum += 1; count++; }
    if (right_outer) { sum += 3; count++; }

    if (count == 0) return last_error;
    return (int8_t)(sum / count);
}

static void reset_turn_counters(void)
{
    center_confirm = 0;
    opposite_confirm = 0;
    turn_hold_count = 0;
    turn_lost_count = 0;
}

static void begin_turn(LineFollowMode direction, LineFollowTurnLevel level)
{
    mode = direction;
    turn_level = level;
    last_turn = (direction == LF_TURN_LEFT) ? -1 : +1;
    reset_turn_counters();
    command_current_turn();
}

static void continue_turn(uint8_t all_white,
                          uint8_t center_pair,
                          uint8_t same_outer,
                          uint8_t opposite_side,
                          int8_t error,
                          int16_t base_speed)
{
    if (turn_hold_count < 255U) turn_hold_count++;

    /*
     * 外侧探头命中，或转弯中连续全白：认为偏差较大/进入直角，
     * 只“升级”到强转弯，不来回降级，减少动作次数。
     */
    if (same_outer) {
        turn_level = LF_LEVEL_CORNER;
        turn_lost_count = 0;
    } else if (all_white) {
        if (turn_lost_count < 255U) turn_lost_count++;
        if (turn_lost_count >= LINE_FOLLOW_LOST_TO_CORNER_MS) {
            turn_level = LF_LEVEL_CORNER;
        }
    } else {
        turn_lost_count = 0;
    }

    /*
     * 中心确认：
     * - 必须先满足最小转弯保持时间；
     * - 必须连续稳定在中间两路；
     * - 同方向外侧仍压线时不算真正转正。
     */
    if (center_pair && !same_outer &&
        turn_hold_count >= LINE_FOLLOW_TURN_MIN_HOLD_MS) {

        if (center_confirm < 255U) center_confirm++;
        opposite_confirm = 0;

        if (center_confirm >= LINE_FOLLOW_CENTER_CONFIRM_MS) {
            mode = LF_STRAIGHT;
            center_confirm = 0;
            lost_count = 0;
            command_straight(base_speed);
            return;
        }

        /* 已接近中心：继续同方向小力度收尾，不立即直行。 */
        command_settle_turn(mode);
        return;
    }

    center_confirm = 0;

    /*
     * 反方向保护：
     * 圆弧边缘的 1~4 ms 抖动不允许立即“左→右→左”反打。
     * 只有反方向连续稳定出现才真正换向。
     */
    if (!all_white && opposite_side && ((mode == LF_TURN_LEFT && error > 0) ||
                                        (mode == LF_TURN_RIGHT && error < 0))) {
        if (opposite_confirm < 255U) opposite_confirm++;

        if (opposite_confirm >= LINE_FOLLOW_OPPOSITE_CONFIRM_MS) {
            LineFollowMode new_direction = (mode == LF_TURN_LEFT) ? LF_TURN_RIGHT
                                                                  : LF_TURN_LEFT;
            LineFollowTurnLevel new_level = LF_LEVEL_ARC;

            begin_turn(new_direction, new_level);
            return;
        }

        /* 等待确认期间只用收尾力度，不猛打当前方向。 */
        command_settle_turn(mode);
        return;
    }

    opposite_confirm = 0;

    if (all_white) {
        /*
         * 丢线时不增加“停/直/反打”等额外动作，只沿当前方向持续找线。
         * 已升级为 CORNER 后保持强转，否则用搜索力度。
         */
        if (turn_level == LF_LEVEL_CORNER) {
            command_corner_turn(mode);
        } else {
            command_search_turn(mode);
        }
        return;
    }

    command_current_turn();
}

void LineFollow_Init(void)
{
    Motor_Init();
    IRTracking_Init();

    mode = LF_STRAIGHT;
    turn_level = LF_LEVEL_ARC;
    last_error = 0;
    last_turn = 0;

    center_confirm = 0;
    opposite_confirm = 0;
    lost_count = 0;
    turn_hold_count = 0;
    turn_lost_count = 0;

    current_left = 0;
    current_right = 0;
    target_left = 0;
    target_right = 0;
    apply_physical_wheels(0, 0);
}

void LineFollow_Run(int16_t base_speed)
{
    uint8_t state = IRTracking_ReadAll();  /* bit0=X1 ... bit3=X4；1=白地 */
    uint8_t x1_black = (state & 0x01U) ? 0U : 1U;
    uint8_t x2_black = (state & 0x02U) ? 0U : 1U;
    uint8_t x3_black = (state & 0x04U) ? 0U : 1U;
    uint8_t x4_black = (state & 0x08U) ? 0U : 1U;

    uint8_t left_outer;
    uint8_t left_inner;
    uint8_t right_inner = x3_black;
    uint8_t right_outer = x4_black;

#if LINE_FOLLOW_SWAP_X1_X2
    left_outer = x2_black;
    left_inner = x1_black;
#else
    left_outer = x1_black;
    left_inner = x2_black;
#endif

    uint8_t on_line = (uint8_t)(left_outer + left_inner + right_inner + right_outer);
    uint8_t all_white = (on_line == 0U);
    uint8_t all_black = (on_line == 4U);
    uint8_t center_pair = (uint8_t)(left_inner && right_inner);
    int8_t error = last_error;

    if (!all_white) {
        error = compute_line_position(left_outer, left_inner, right_inner, right_outer);
        last_error = error;
    }

    /*
     * 四路全黑：
     * - 直行状态：按十字/宽黑线直接高速通过；
     * - 已经在转弯：不突然改成直行，保持原转向，避免直角拐到一半被“路口形状”打断。
     */
    if (all_black) {
        lost_count = 0;

        if (mode == LF_STRAIGHT) {
            command_straight(base_speed);
        } else {
            continue_turn(0U,
                          center_pair,
                          (mode == LF_TURN_LEFT) ? left_outer : right_outer,
                          0U,
                          error,
                          base_speed);
        }

        update_wheel_output();
        return;
    }

    if (mode == LF_STRAIGHT) {
        reset_turn_counters();

        if (center_pair) {
            lost_count = 0;
            command_straight(base_speed);
        } else if (all_white) {
            if (lost_count < 255U) lost_count++;

            if (lost_count <= LINE_FOLLOW_LOST_GRACE_MS) {
                command_straight(base_speed);
            } else if (last_turn < 0) {
                begin_turn(LF_TURN_LEFT, LF_LEVEL_ARC);
                command_search_turn(LF_TURN_LEFT);
            } else if (last_turn > 0) {
                begin_turn(LF_TURN_RIGHT, LF_LEVEL_ARC);
                command_search_turn(LF_TURN_RIGHT);
            } else {
                /* 启动时完全找不到线：不盲目前冲。 */
                set_target_wheels(0, 0);
            }
        } else {
            LineFollowTurnLevel level;
            lost_count = 0;

            if (error < 0) {
                level = left_outer ? LF_LEVEL_CORNER : LF_LEVEL_ARC;
                begin_turn(LF_TURN_LEFT, level);
            } else if (error > 0) {
                level = right_outer ? LF_LEVEL_CORNER : LF_LEVEL_ARC;
                begin_turn(LF_TURN_RIGHT, level);
            } else {
                /* 左右对称图样按直行处理，避免无意义摆动。 */
                command_straight(base_speed);
            }
        }
    } else {
        uint8_t same_outer = (mode == LF_TURN_LEFT) ? left_outer : right_outer;
        uint8_t opposite_side = (mode == LF_TURN_LEFT)
                              ? (uint8_t)(right_inner || right_outer)
                              : (uint8_t)(left_inner || left_outer);

        lost_count = 0;
        continue_turn(all_white, center_pair, same_outer, opposite_side, error, base_speed);
    }

    update_wheel_output();
}

int8_t LineFollow_GetError(void)
{
    return last_error;
}

int8_t LineFollow_GetTurnDirection(void)
{
    return (int8_t)mode;
}
