/*
 * follow_vision.c — 循迹 + 视觉路牌综合任务
 *
 * 正常循迹行驶 → K210 YOLOv2 路牌识别触发驾驶行为
 *
 * 路牌指令映射 (7 类):
 *   H  (鸣笛)           → 蜂鸣器响 (自动关闭)
 *   L  (左转)           → 三层保险入岛 → 岛内循迹 → 旋转出岛 → 恢复循迹
 *   1  (停车位类型1/PARK1) → 停车
 *   2  (停车位类型2/PARK2) → 停车
 *   R  (右转)           → 三层保险入岛 → 岛内循迹 → 旋转出岛 → 恢复循迹
 *   W  (限速)           → 循迹降速
 *   F  (解除限速)       → 恢复循迹 (速度 FV_LINE_RELEASE_SPEED)
 *
 * 入环岛三层保险：
 *   1) 编码器空间门控 — 视觉提前预告方向，但编码器累计走过设定距离后才解锁路口检测
 *   2) 方向相关强路口图样 — LEFT 只认左三探头, RIGHT 只认右三探头
 *   3) 连续稳定确认 — 强路口/全白必须持续若干 ms, 过滤毛刺
 *
 * 出岛旋转时传感器初始有信号, 需先脱当前线再找新线。
 */

#include "follow_vision.h"
#include "follow_vision_config.h"
#include "irtracking.h"
#include "line_follow.h"
#include "k210_comm.h"
#include "motor.h"
#include "encoder.h"
#include "led.h"
#include "led_config.h"
#include "buzz.h"

#if FV_OLED_ENABLE
#include "oled.h"
#endif

/* ---- state machine ---- */

typedef enum {
    FV_FOLLOW,          /* 正常循迹 */
    FV_ISLAND_WAIT,     /* 已收到 L/R, 编码器门控+路口图样确认 */
    FV_ISLAND_STOP,     /* 路口确认: 停车 1s */
    FV_ISLAND_ROT1,     /* 入岛旋转: 先脱旧线再找新线 */
    FV_ISLAND_FOLLOW,   /* 岛内循迹: 沿环岛黑线行驶 */
    FV_ISLAND_ROT2,     /* 出岛旋转: 先脱当前线再找新线 (传感器初始有信号) */
    FV_SLOW,            /* 限速循迹 */
    FV_PARK,            /* 停车 (PARK1/PARK2) */
    FV_HORN             /* 鸣笛 (自动关闭) */
} FV_State;

static FV_State  fv_state        = FV_FOLLOW;
static FV_State  turn_resume_state = FV_FOLLOW;  /* 入岛完成后恢复的状态 */
static int8_t    fv_dir          = 0;          /* 环岛转向方向: -1=左, +1=右 */
static uint16_t  phase_ms        = 0;          /* 当前阶段计时器 (ms) */
static uint16_t  hb_cnt          = 0;          /* K210 心跳计数 (ms) */
static char      last_sign[3]    = "-";        /* 最近路牌指令 */
static uint16_t  fv_run_speed    = FV_LINE_BASE_SPEED; /* 当前循迹速度 (W/F 可修改) */
static uint8_t   lost_line       = 0;          /* ROT2 子状态: 0=等待脱线, 1=已脱线找新线 */
static FV_State  horn_return     = FV_FOLLOW;  /* HORN 结束后返回的状态 */

/* ---- 入岛路口检测变量 ---- */

static uint16_t  wide_cnt        = 0;   /* 强路口连续计数 */
static uint16_t  white_cnt       = 0;   /* 全白连续计数 */
static uint16_t  early_white_cnt = 0;   /* 早期全白保持直行计数 */
static uint16_t  depart_cnt      = 0;   /* 脱离旧路口计数 */
static uint16_t  line_cnt        = 0;   /* 找到新线稳定计数 */
static uint8_t   turn_departed   = 0;   /* 是否已脱离旧路口 */

/*
 * 从收到 L/R 开始累计四轮编码器"运动量"的平均值。
 * 只有累计到 FV_ISLAND_ARM_DISTANCE_COUNTS 后, 路口检测才会解锁。
 * 这样视觉可以提前看到路牌, 但路面干扰不能立即触发转弯程序。
 */
static uint32_t  turn_travel_counts   = 0;
static uint8_t   turn_junction_armed  = 0;

/* ---- helpers ---- */

static const char *fv_state_names[] = {
    "FOLLOW", "ISL_W", "ISL_S", "ISL_R1", "ISL_FLW", "ISL_R2", "SLOW", "PARK", "HORN"
};

/** Match sign payload — all K210 commands are single-char: H/L/R/W/F/1/2 */
static int sign_eq(const char *msg, const char *sign)
{
    int i = 0;
    while (sign[i] != '\0') {
        if (msg[i] != sign[i]) return 0;
        i++;
    }
    return 1;
}

/** Record last sign for display (max 2 chars + null) */
static void record_sign(const char *sign)
{
    int i = 0;
    while (i < 2 && sign[i] != '\0') {
        last_sign[i] = sign[i];
        i++;
    }
    last_sign[i] = '\0';
}

/** Count how many of the 4 IR sensors see black (cleared bit = black). */
static uint8_t fv_black_count(void)
{
    uint8_t state = IRTracking_ReadAll();
    uint8_t count = 0;

    if ((state & 0x01U) == 0U) count++;  /* X1 */
    if ((state & 0x02U) == 0U) count++;  /* X2 */
    if ((state & 0x04U) == 0U) count++;  /* X3 */
    if ((state & 0x08U) == 0U) count++;  /* X4 */

    return count;
}

/*
 * 计算本 1 ms 内四个编码器运动量的平均绝对值。
 * 不直接使用 Encoder_GetTotal(), 避免影响其它模块, 也不要求四轮
 * 在循迹修正时必须同向。
 */
static uint16_t fv_encoder_motion_step(void)
{
    int16_t delta[4];
    uint32_t sum = 0;

    Encoder_GetAllDeltas(delta);

    for (uint8_t i = 0; i < 4U; i++) {
        int32_t v = delta[i];
        if (v < 0) v = -v;
        sum += (uint32_t)v;
    }

    return (uint16_t)(sum / 4U);
}

/** 累计编码器运动量, 满足条件后解锁路口检测。 */
static void fv_update_turn_distance(void)
{
    uint16_t step = fv_encoder_motion_step();

    if ((0xFFFFFFFFUL - turn_travel_counts) < (uint32_t)step) {
        turn_travel_counts = 0xFFFFFFFFUL;
    } else {
        turn_travel_counts += (uint32_t)step;
    }

    if (!turn_junction_armed &&
        phase_ms >= FV_ISLAND_ARM_MIN_MS &&
        turn_travel_counts >= FV_ISLAND_ARM_DISTANCE_COUNTS) {

        turn_junction_armed = 1U;

        /* 解锁瞬间重新开始计数, 避免把解锁前的噪声带进确认窗口。 */
        wide_cnt = 0;
        white_cnt = 0;
        early_white_cnt = 0;
    }
}

/*
 * 方向相关的"强路口"图样。
 *
 * 物理顺序: X2(左外) X1(左内) X3(右内) X4(右外)
 *
 * LEFT  只接受: X2 + X1 + X3 (或四路全黑)
 * RIGHT 只接受: X1 + X3 + X4 (或四路全黑)
 *
 * 相比"任意 >=3 路黑", 这样不会因为错误一侧的宽黑斑
 * 就触发已经记录的 L/R 动作。
 */
static uint8_t fv_requested_junction_seen(uint8_t state, int8_t dir)
{
    uint8_t x1_black = ((state & 0x01U) == 0U) ? 1U : 0U;
    uint8_t x2_black = ((state & 0x02U) == 0U) ? 1U : 0U;
    uint8_t x3_black = ((state & 0x04U) == 0U) ? 1U : 0U;
    uint8_t x4_black = ((state & 0x08U) == 0U) ? 1U : 0U;

    if (x1_black && x2_black && x3_black && x4_black) {
        return 1U;
    }

    if (dir < 0) {
        return (x2_black && x1_black && x3_black) ? 1U : 0U;
    }

    return (x1_black && x3_black && x4_black) ? 1U : 0U;
}

/*
 * 认为重新找到路线:
 *   - X1 或 X3 至少一只看到黑线;
 *   - 同时总黑线数不超过 2, 避免把宽路口本身误当作新路线。
 */
static uint8_t fv_center_line_seen(void)
{
    uint8_t state = IRTracking_ReadAll();

    uint8_t x1_black = ((state & 0x01U) == 0U) ? 1U : 0U;
    uint8_t x3_black = ((state & 0x04U) == 0U) ? 1U : 0U;

    uint8_t black_count = fv_black_count();

    if ((x1_black || x3_black) &&
        black_count <= FV_ISLAND_DEPART_MAX_BLACK) {
        return 1U;
    }

    return 0U;
}

/** 当前循迹速度 (考虑 SLOW 状态) */
static int16_t fv_follow_speed(void)
{
    return (turn_resume_state == FV_SLOW)
        ? FV_LINE_SLOW_SPEED
        : FV_LINE_BASE_SPEED;
}

/** 路口接近速度: 取循迹速度和 ISLAND_APPROACH_SPEED 的较小值 */
static int16_t fv_junction_approach_speed(void)
{
    int16_t speed = fv_follow_speed();

    if (speed > FV_ISLAND_APPROACH_SPEED) {
        speed = FV_ISLAND_APPROACH_SPEED;
    }

    return speed;
}

/**
 * 使用 car_diff_turn() 的"物理左右侧"接口,
 * 不使用 pwm_car_rotate_left/right (接线反向问题)。
 */
static void fv_do_physical_turn(int8_t dir)
{
    int16_t speed = FV_ISLAND_ROT_SPEED;

    if (dir < 0) {
        /* 物理左转: 左侧后退, 右侧前进 */
        car_diff_turn((int16_t)-speed, speed);
    } else {
        /* 物理右转: 左侧前进, 右侧后退 */
        car_diff_turn(speed, (int16_t)-speed);
    }
}

/** 重置所有路口检测计数器 */
static void fv_reset_turn_counters(void)
{
    wide_cnt = 0;
    white_cnt = 0;
    early_white_cnt = 0;
    depart_cnt = 0;
    line_cnt = 0;
    turn_departed = 0;

    turn_travel_counts = 0;
    turn_junction_armed = 0;
}

/** 开始等待路口 (收到 L/R 时调用) */
static void fv_start_island_wait(int8_t dir, FV_State resume_state)
{
    fv_dir = (dir < 0) ? -1 : +1;
    turn_resume_state = (resume_state == FV_SLOW) ? FV_SLOW : FV_FOLLOW;

    fv_reset_turn_counters();

    /* 亮对应转向灯 */
    if (dir < 0) {
        LED_Set(LED_PRESET_TURN_LEFT);
    } else {
        LED_Set(LED_PRESET_TURN_RIGHT);
    }

    fv_state = FV_ISLAND_WAIT;
    phase_ms = 0;
}

/** 超时取消待转任务, 恢复正常循迹 */
static void fv_cancel_turn_request(void)
{
    LineFollow_Init();
    K210Comm_ClearFlag();

    fv_dir = 0;
    fv_reset_turn_counters();

    LED_Set(LED_PRESET_FORWARD);

    fv_state = turn_resume_state;
    phase_ms = 0;
}

/** 路口确认, 进入停车 */
static void fv_enter_island_stop(void)
{
    /*
     * 从这一刻开始不再调用 LineFollow。
     * 先能耗制动, 保证不会再被旧循迹输出推着冲出路口。
     */
    car_brake();

    wide_cnt = 0;
    white_cnt = 0;

    fv_state = FV_ISLAND_STOP;
    phase_ms = 0;
}

/** 停车结束, 进入入岛旋转 */
static void fv_enter_island_rot1(void)
{
    /*
     * 直接进入物理方向转弯。
     * turn_departed / line_cnt 重新置零, 确保不会把旧路口当成新路线。
     */
    depart_cnt = 0;
    line_cnt = 0;
    turn_departed = 0;

    fv_state = FV_ISLAND_ROT1;
    phase_ms = 0;

    fv_do_physical_turn(fv_dir);
}

/* ---- public API ---- */

void FollowVision_Init(void)
{
    Motor_Init();
    IRTracking_Init();
    K210Comm_Init();
    LineFollow_Init();

    fv_state        = FV_FOLLOW;
    turn_resume_state = FV_FOLLOW;
    fv_dir          = 0;
    phase_ms        = 0;
    hb_cnt          = 0;
    fv_run_speed    = FV_LINE_BASE_SPEED;
    lost_line       = 0;
    horn_return     = FV_FOLLOW;
    last_sign[0]    = '-';
    last_sign[1]    = '\0';

    fv_reset_turn_counters();

#if FV_OLED_ENABLE
    OLED_Init();
    OLED_Clear();
    OLED_GotoXY(0, 0);
    OLED_Puts("FV: FOLLOW", &Font_7x10, OLED_COLOR_WHITE);
    OLED_GotoXY(0, 10);
    OLED_Puts("Sign:-", &Font_7x10, OLED_COLOR_WHITE);
    OLED_Update();
#endif
}

void FollowVision_Tick(void)
{
    /* ---- K210 心跳 ---- */
    if (++hb_cnt >= FV_K210_HEARTBEAT_MS) {
        hb_cnt = 0;
        K210Comm_SendFrame("alive");
    }

    phase_ms++;

    /* ---- 状态机 ---- */
    switch (fv_state) {

    case FV_FOLLOW:
        /* 正常循迹, 同时检查 K210 路牌指令 */
        LineFollow_Run(fv_run_speed);

        if (K210Comm_HasMessage()) {
            const char *msg = K210Comm_GetMessage();
            K210Comm_ClearFlag();

            if (sign_eq(msg, "L")) {
                /* 左转 → 三层保险入岛 */
                record_sign("L");
                fv_start_island_wait(-1, FV_FOLLOW);

            } else if (sign_eq(msg, "R")) {
                /* 右转 → 三层保险入岛 */
                record_sign("R");
                fv_start_island_wait(+1, FV_FOLLOW);

            } else if (sign_eq(msg, "H")) {
                /* 鸣笛 → 蜂鸣器响, 自动关闭 */
                record_sign("H");
                Buzz_On();
                horn_return = FV_FOLLOW;
                fv_state = FV_HORN;
                phase_ms = 0;

            } else if (sign_eq(msg, "W")) {
                /* 限速 → 循迹降速 */
                record_sign("W");
                fv_state = FV_SLOW;
                phase_ms = 0;

            } else if (sign_eq(msg, "1") || sign_eq(msg, "2")) {
                /* 停车 (PARK1/PARK2) — K210 裸字符: 1=PARK1, 2=PARK2 */
                record_sign(msg);
                pwm_car_stop();
                LED_Set(LED_PRESET_STOP);
                Buzz_On();
                fv_state = FV_PARK;
                phase_ms = 0;
            }
        }
        break;

    /* ========================================================
     * LEFT / RIGHT already known, wait for actual junction
     * 三层保险: 编码器空间门控 + 方向相关强路口 + 稳定确认
     * ======================================================== */
    case FV_ISLAND_WAIT:
    {
        uint8_t state = IRTracking_ReadAll();
        uint8_t black_count = fv_black_count();
        uint8_t strong_junction;
        uint8_t white_allowed;

        /*
         * ISLAND_WAIT 期间不接受新的视觉动作, 防止同一块 L/R 路牌在
         * K210 短暂丢检后重发, 留下"转完以后立刻再转一次"的陈旧消息。
         */
        if (K210Comm_HasMessage()) {
            K210Comm_ClearFlag();
        }

        /*
         * 第一层保险: 空间门控。
         * 视觉可以很早识别, 但在编码器累计走过设定距离以前,
         * 红外图样无权启动路口停车/转弯状态机。
         */
        fv_update_turn_distance();

        if (!turn_junction_armed) {
            wide_cnt = 0;
            white_cnt = 0;

            /*
             * 提前阶段的短全白更可能是地面反光/缝隙, 而不是目标路口。
             * 先短时间保持直行, 避免 LineFollow 在 2 ms 后立刻按
             * last_turn 启动丢线搜索; 若全白持续很久, 才交还 LineFollow
             * 做真正的丢线恢复。
             */
            if (black_count == 0U) {
                if (early_white_cnt < 0xFFFFU) early_white_cnt++;

                if (early_white_cnt <= FV_ISLAND_EARLY_WHITE_HOLD_MS) {
                    pwm_car_forward(fv_junction_approach_speed());
                } else {
                    LineFollow_Run(fv_follow_speed());
                }
            } else {
                early_white_cnt = 0;
                LineFollow_Run(fv_follow_speed());
            }

            if (phase_ms >= FV_ISLAND_WAIT_TIMEOUT_MS) {
                fv_cancel_turn_request();
            }
            break;
        }

        /*
         * 第二层保险: 方向相关的路口图样。
         * 不再使用"任意 >=3 黑"作为直接触发条件, 而是要求宽黑区域
         * 覆盖到已经请求的那一侧。
         */
        strong_junction = fv_requested_junction_seen(state, fv_dir);

        /*
         * 四路全白是最容易被地面干扰伪造的图样, 因此比强路口图样
         * 还要多走一段距离才允许参与触发。
         */
        white_allowed =
            (turn_travel_counts >=
             (FV_ISLAND_ARM_DISTANCE_COUNTS +
              FV_ISLAND_WHITE_EXTRA_DISTANCE_COUNTS)) ? 1U : 0U;

        if (strong_junction) {
            if (wide_cnt < 0xFFFFU) wide_cnt++;
            white_cnt = 0;
            early_white_cnt = 0;

            pwm_car_forward(fv_junction_approach_speed());

        } else if (black_count == 0U && white_allowed) {
            if (white_cnt < 0xFFFFU) white_cnt++;
            wide_cnt = 0;
            early_white_cnt = 0;

            pwm_car_forward(fv_junction_approach_speed());

        } else {
            wide_cnt = 0;
            white_cnt = 0;

            if (black_count == 0U) {
                /*
                 * 已解锁但尚未达到全白的额外距离门槛:
                 * 短全白仍视为干扰, 先保持直行, 持续太久再允许普通找线。
                 */
                if (early_white_cnt < 0xFFFFU) early_white_cnt++;

                if (early_white_cnt <= FV_ISLAND_EARLY_WHITE_HOLD_MS) {
                    pwm_car_forward(fv_junction_approach_speed());
                } else {
                    LineFollow_Run(fv_follow_speed());
                }
            } else {
                early_white_cnt = 0;
                LineFollow_Run(fv_follow_speed());
            }
        }

        if (wide_cnt >= FV_ISLAND_WIDE_CONFIRM_MS ||
            (white_allowed && white_cnt >= FV_ISLAND_WHITE_CONFIRM_MS)) {

            fv_enter_island_stop();

        } else if (phase_ms >= FV_ISLAND_WAIT_TIMEOUT_MS) {
            /*
             * 路牌过早/误识别, 或一直没有遇到合法路口:
             * 超时取消, 不允许永久带着一个待转方向跑。
             */
            fv_cancel_turn_request();
        }
        break;
    }

    /* ========================================================
     * Confirmed junction: stop 1 second
     * ======================================================== */
    case FV_ISLAND_STOP:
        if (K210Comm_HasMessage()) {
            K210Comm_ClearFlag();
        }

        /*
         * First short brake pulse for fast deceleration,
         * then zero PWM for the remainder of the stop.
         */
        if (phase_ms <= FV_ISLAND_BRAKE_MS) {
            car_brake();
        } else {
            pwm_car_stop();
        }

        if (phase_ms >= FV_ISLAND_STOP_MS) {
            fv_enter_island_rot1();
        }
        break;

    /* ========================================================
     * 入岛旋转: 先脱旧路口再找新线
     * 不调用 LineFollow, 电机完全由 FollowVision 独占
     * ======================================================== */
    case FV_ISLAND_ROT1:
    {
        uint8_t black_count = fv_black_count();

        if (K210Comm_HasMessage()) {
            K210Comm_ClearFlag();
        }

        fv_do_physical_turn(fv_dir);

        /*
         * First make sure we have actually left the old wide junction.
         * This blocks immediate false success caused by the old line still
         * being under the center sensors when rotation starts.
         */
        if (!turn_departed) {
            if (black_count <= FV_ISLAND_DEPART_MAX_BLACK) {
                if (depart_cnt < 0xFFFFU) depart_cnt++;
            } else {
                depart_cnt = 0;
            }

            if (depart_cnt >= FV_ISLAND_DEPART_CONFIRM_MS) {
                turn_departed = 1U;
                line_cnt = 0;
            }
        }

        /*
         * After a minimum physical turn time, accept a new line only when it
         * is located in the center sensor zone and remains stable.
         */
        if (turn_departed &&
            phase_ms >= FV_ISLAND_MIN_MS &&
            fv_center_line_seen()) {

            if (line_cnt < 0xFFFFU) line_cnt++;

        } else {
            line_cnt = 0;
        }

        if (line_cnt >= FV_ISLAND_LINE_CONFIRM_MS) {
            /* 找到新线 → 停止旋转 → 岛内循迹 */
            pwm_car_stop();
            LineFollow_Init();

            fv_state = FV_ISLAND_FOLLOW;
            phase_ms = 0;

        } else if (phase_ms >= FV_ISLAND_MAX_MS) {
            /*
             * Safety timeout: never spin forever.
             * 强制进入岛内循迹, 让车继续跑。
             */
            pwm_car_stop();
            LineFollow_Init();

            fv_state = FV_ISLAND_FOLLOW;
            phase_ms = 0;
        }
        break;
    }

    case FV_ISLAND_FOLLOW:
        /* 岛内循迹: 沿环岛黑线行驶 */
        LineFollow_Run(FV_ISLAND_FOLLOW_SPEED);

        if (phase_ms >= FV_ISLAND_RUN_MS) {
            /* 岛内循迹结束 → 原地旋转退出环岛 */
            if (fv_dir < 0) {
                /* 左转: 左轮后退, 右轮前进 → 物理左旋 */
                car_diff_turn(-FV_ISLAND_ROT_SPEED, FV_ISLAND_ROT_SPEED);
            } else {
                /* 右转: 左轮前进, 右轮后退 → 物理右旋 */
                car_diff_turn(FV_ISLAND_ROT_SPEED, -FV_ISLAND_ROT_SPEED);
            }
            lost_line = 0;
            fv_state = FV_ISLAND_ROT2;
            phase_ms = 0;
        }
        break;

    case FV_ISLAND_ROT2:
        /* 出岛旋转: 先脱当前线再找新线 */
        if (!lost_line) {
            /* 阶段1: 等待脱离当前黑线 (所有传感器变白) */
            if (IRTracking_ReadAll() == 0x0F) {
                lost_line = 1;
            }
        } else {
            /* 阶段2: 寻找新黑线 */
            if (IRTracking_ReadAll() != 0x0F) {
                /* 找到新黑线 → 停止旋转 → 恢复正常循迹 */
                pwm_car_stop();
                LineFollow_Init();

                fv_dir = 0;
                LED_Set(LED_PRESET_FORWARD);

                fv_state = FV_FOLLOW;
                phase_ms = 0;
            }
        }
        break;

    case FV_SLOW:
        /* 限速循迹 → 检查 K210 指令 */
        LineFollow_Run(FV_LINE_SLOW_SPEED);

        if (K210Comm_HasMessage()) {
            const char *msg = K210Comm_GetMessage();
            K210Comm_ClearFlag();

            if (sign_eq(msg, "F")) {
                /* 解除限速 → 恢复循迹 (速度 FV_LINE_RELEASE_SPEED) */
                record_sign("F");
                Buzz_Off();
                fv_run_speed = FV_LINE_RELEASE_SPEED;
                fv_state = FV_FOLLOW;
                phase_ms = 0;

            } else if (sign_eq(msg, "L")) {
                /* 左转 → 三层保险入岛 (从 SLOW 状态) */
                record_sign("L");
                fv_start_island_wait(-1, FV_SLOW);

            } else if (sign_eq(msg, "R")) {
                /* 右转 → 三层保险入岛 (从 SLOW 状态) */
                record_sign("R");
                fv_start_island_wait(+1, FV_SLOW);

            } else if (sign_eq(msg, "1") || sign_eq(msg, "2")) {
                /* 停车 (PARK1/PARK2) — K210 裸字符: 1=PARK1, 2=PARK2 */
                record_sign(msg);
                pwm_car_stop();
                LED_Set(LED_PRESET_STOP);
                Buzz_On();
                fv_state = FV_PARK;
                phase_ms = 0;

            } else if (sign_eq(msg, "H")) {
                /* 鸣笛 — 鸣笛结束后回到 SLOW 继续限速 */
                record_sign("H");
                Buzz_On();
                horn_return = FV_SLOW;
                fv_state = FV_HORN;
                phase_ms = 0;
            }
        }
        break;

    case FV_PARK:
        /* 停车 → 检查 K210 指令恢复 */
        if (K210Comm_HasMessage()) {
            const char *msg = K210Comm_GetMessage();
            K210Comm_ClearFlag();

            if (sign_eq(msg, "F")) {
                /* 解除限速 / 绿灯 → 恢复循迹 */
                record_sign("F");
                Buzz_Off();
                fv_run_speed = FV_LINE_RELEASE_SPEED;
                LineFollow_Init();
                fv_state = FV_FOLLOW;
                phase_ms = 0;
            }
        }
        break;

    case FV_HORN:
        /* 鸣笛 → 循迹继续, 蜂鸣器自动关闭 */
        LineFollow_Run(fv_run_speed);
        if (phase_ms >= FV_HORN_MS) {
            Buzz_Off();
            fv_state = horn_return;
            phase_ms = 0;
        }
        break;
    }

    /* ---- OLED 显示 ---- */
#if FV_OLED_ENABLE
    {
        static uint16_t disp_cnt = 0;
        if (++disp_cnt < FV_DISPLAY_MS) return;
        disp_cnt = 0;

        OLED_Clear();
        OLED_GotoXY(0, 0);
        OLED_Puts("FV:", &Font_7x10, OLED_COLOR_WHITE);
        OLED_Puts(fv_state_names[fv_state], &Font_7x10, OLED_COLOR_WHITE);
        OLED_GotoXY(0, 10);
        OLED_Puts("Sign:", &Font_7x10, OLED_COLOR_WHITE);
        OLED_Puts(last_sign, &Font_7x10, OLED_COLOR_WHITE);
        OLED_Update();
    }
#endif
}

const char *FollowVision_GetStateName(void)
{
    return fv_state_names[fv_state];
}

const char *FollowVision_GetLastSign(void)
{
    return last_sign;
}

uint32_t FollowVision_GetTurnTravelCounts(void)
{
    return turn_travel_counts;
}

uint8_t FollowVision_IsTurnJunctionArmed(void)
{
    return turn_junction_armed;
}
