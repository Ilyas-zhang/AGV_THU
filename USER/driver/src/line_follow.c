/*
 * line_follow.c — 循迹运动控制（加权误差 + Kp 比例差速 + EMA 滤波）
 *
 * 传感器布局（左→右）：X2  [X1 X3]  X4
 * X1/X3 物理间距太近，合并为等效中心传感器 mid = X1∨X3
 * 加权位置: X2=-3, mid=0, X4=+3
 *
 * error = (-3)*x2 + (+3)*x4
 *
 * EMA 低通滤波（定点累加器 ×256，避免整型截断/放大误差）:
 *   ema_acc += (target - ema_acc) / N    （target = error << 8）
 *   filtered = ema_acc >> 8
 *
 * 比例差速:
 *   left_speed  = base_speed + Kp * filtered_error
 *   right_speed = base_speed - Kp * filtered_error
 *   快侧 clamp 到 [MIN_START, MAX_SPEED]   ← 必须能驱动
 *   慢侧 clamp 到 [0, MAX_SPEED]            ← 允许停转以增大差速
 *
 * 脱线恢复: 全白时按 last_error 方向原地旋转找回
 * 十字路口: 全黑时 error=0, 直行
 */

#include "line_follow.h"
#include "line_follow_config.h"
#include "irtracking.h"
#include "motor.h"
#include "motor_config.h"

/* ---- EMA 定点参数 ---- */
#define EMA_FP_SHIFT   8           /* 8 位小数 (×256) */
#define EMA_FP_SCALE   (1 << 8)    /* 256 */

/* ---- state ---- */
static int8_t  last_error    = 0;   /* 上次原始误差: negative=偏左, positive=偏右 */
static int32_t ema_acc       = 0;   /* EMA 定点累加器 (×256) */
static int16_t filtered_err  = 0;   /* EMA 滤波后误差（整数，与原始同量纲） */

/* ---- clamping helper ---- */
static inline int16_t clamp(int16_t val, int16_t lo, int16_t hi)
{
    if (val < lo) return lo;
    if (val > hi) return hi;
    return val;
}

/* ========== 初始化 ========== */

void LineFollow_Init(void)
{
    Motor_Init();
    IRTracking_Init();
    last_error   = 0;
    ema_acc      = 0;
    filtered_err = 0;
}

/* ========== 运行 ========== */

void LineFollow_Run(int16_t base_speed)
{
    uint8_t state = IRTracking_ReadAll();   /* bit0=X1..bit3=X4, 0=黑线, 1=白地 */

    /* 提取各路: xi=1 表示在线上(黑线) */
    uint8_t x1 = (state & 0x01) ? 0 : 1;
    uint8_t x2 = (state & 0x02) ? 0 : 1;
    uint8_t x3 = (state & 0x04) ? 0 : 1;
    uint8_t x4 = (state & 0x08) ? 0 : 1;

    /* 合并 X1/X3 为等效中心传感器 (物理间距太近，合并为 1 路) */
    uint8_t mid = (x1 || x3) ? 1 : 0;

    uint8_t on_line = mid + x2 + x4;

    if (on_line == 0) {
        /* ---- 全白：脱线，按 last_error 方向旋转找回 ---- */
        if (last_error > 0) {
            pwm_car_rotate_left(LINE_FOLLOW_ROTATE_SPEED);
        } else if (last_error < 0) {
            pwm_car_rotate_right(LINE_FOLLOW_ROTATE_SPEED);
        } else {
            pwm_car_forward(LINE_FOLLOW_ROTATE_SPEED);
        }
        return;
    }

    /* ---- 计算加权误差 ---- */
    /* 3 等效传感器: X2(-3), mid(0), X4(+3) */
    int8_t error = (int8_t)(-3 * x2 + 3 * x4);
    last_error = error;

    /* ---- EMA 低通滤波 (定点 ×256) ---- */
    /*  标准 EMA: f_new = f_old + (target - f_old) / N
     *  定点实现: ema_acc 是 f × 256
     *  稳态: ema_acc → error × 256, filtered → error ✓
     *  衰减: error=0 时, N=2 → 3 ticks 从 ±3 衰减到 0
     */
    {
        int32_t target = (int32_t)error << EMA_FP_SHIFT;
        ema_acc += (target - ema_acc) / LF_EMA_ALPHA;
        /* 读出整数值 (对称四舍五入: 正负一致) */
        int32_t abs_val = ema_acc >= 0 ? ema_acc : -ema_acc;
        int16_t rounded = (int16_t)((abs_val + EMA_FP_SCALE / 2) >> EMA_FP_SHIFT);
        filtered_err = ema_acc >= 0 ? rounded : -rounded;
    }

    /* ---- 比例差速 ---- */
    int16_t adj = (int16_t)LINE_FOLLOW_KP * filtered_err;

    /*
     * 快侧（加速侧）: 必须 ≥ 电机最低启动速度，否则车轮不转
     * 慢侧（减速侧）: 允许降到 0 — 差速转向不需要慢侧驱动，
     *   慢侧停转反而增大差速，转向更灵敏
     */
    int16_t left_speed;
    int16_t right_speed;

    if (adj >= 0) {
        /* error ≥ 0: 线偏右 → 左快右慢 */
        left_speed  = clamp(base_speed + adj, LINE_FOLLOW_MIN_SPEED, LINE_FOLLOW_MAX_SPEED);
        right_speed = clamp(base_speed - adj, 0, LINE_FOLLOW_MAX_SPEED);
    } else {
        /* error < 0: 线偏左 → 左慢右快 */
        left_speed  = clamp(base_speed + adj, 0, LINE_FOLLOW_MAX_SPEED);
        right_speed = clamp(base_speed - adj, LINE_FOLLOW_MIN_SPEED, LINE_FOLLOW_MAX_SPEED);
    }

    /* ---- 应用差速：左电机(left_speed)，右电机(right_speed) ---- */
    pwm_motor1_forward(MOTOR1_PWM(left_speed));
    pwm_motor2_forward(MOTOR2_PWM(left_speed));
    pwm_motor3_forward(MOTOR3_PWM(right_speed));
    pwm_motor4_forward(MOTOR4_PWM(right_speed));
}

/* ========== 调试接口 ========== */

int8_t LineFollow_GetError(void)
{
    return last_error;
}

/**
 * @brief  Get EMA-filtered error value.
 * @retval Filtered error (same units as raw error, range ≈ -3~+3)
 */
int16_t LineFollow_GetFilteredError(void)
{
    return filtered_err;
}
