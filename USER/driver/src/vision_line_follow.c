/*
 * vision_line_follow.c — 视觉循迹驱动（K210 摄像头 + 比例差速）
 *
 * K210 发送 $e<value># 帧:
 *   value: -100~+100 (负=线偏左, 正=偏右), 999=脱线
 *
 * 接收后 EMA 滤波 → Kp 比例差速:
 *   left_speed  = base + Kp * filtered
 *   right_speed = base - Kp * filtered
 *   快侧 ≥ MIN_SPEED, 慢侧 ≥ 0
 *
 * 脱线/超时 → 按 last_error 方向旋转找回
 */

#include "vision_line_follow.h"
#include "vision_line_follow_config.h"
#include "k210_comm.h"
#include "motor.h"
#include "motor_config.h"

/* ---- state ---- */
static int16_t  raw_error     = 0;     /* 最近原始误差 */
static int16_t  filtered_err  = 0;     /* EMA 滤波后误差 */
static int16_t  last_valid    = 0;     /* 上次有效误差 (用于脱线旋转方向) */
static uint8_t  offline       = 0;     /* 脱线标志 */
static uint16_t no_msg_cnt    = 0;     /* 无消息计数 (ms) */
static uint16_t hb_cnt        = 0;     /* 心跳计数 (ms) */

/* ---- clamping helper ---- */
static inline int16_t clamp(int16_t val, int16_t lo, int16_t hi)
{
    if (val < lo) return lo;
    if (val > hi) return hi;
    return val;
}

/* ---- parse signed integer from string ---- */
static int16_t parse_int(const char *s)
{
    int16_t val = 0;
    int i = 0;
    int sign = 1;

    if (s[i] == '-') { sign = -1; i++; }
    else if (s[i] == '+') { i++; }

    while (s[i] >= '0' && s[i] <= '9') {
        val = val * 10 + (s[i] - '0');
        i++;
    }

    return val * sign;
}

/* ========== 初始化 ========== */

void VisionLineFollow_Init(void)
{
    K210Comm_Init();
    Motor_Init();

    raw_error    = 0;
    filtered_err = 0;
    last_valid   = 0;
    offline      = 0;
    no_msg_cnt   = 0;
    hb_cnt       = 0;
}

/* ========== 周期运行 ========== */

void VisionLineFollow_Tick(void)
{
    /* ---- 处理 K210 消息 ---- */
    if (K210Comm_HasMessage()) {
        const char *msg = K210Comm_GetMessage();
        K210Comm_ClearFlag();
        no_msg_cnt = 0;   /* 收到消息，复位超时 */

        if (msg[0] == 'e') {
            int16_t val = parse_int(&msg[1]);

            if (val >= 999 || val <= -999) {
                /* 脱线 */
                offline = 1;
            } else {
                /* 正常误差 */
                raw_error = val;
                last_valid = val;
                offline = 0;

                /* EMA 滤波 */
                int32_t raw32  = (int32_t)val * VLF_EMA_ALPHA;
                int32_t prev32 = (int32_t)filtered_err * (VLF_EMA_ALPHA - 1);
                filtered_err = (int16_t)((raw32 + prev32 + VLF_EMA_ALPHA / 2) / VLF_EMA_ALPHA);
            }
        }
    } else {
        /* 无消息，超时检测 */
        if (++no_msg_cnt >= VLF_TIMEOUT_MS) {
            offline = 1;
            no_msg_cnt = VLF_TIMEOUT_MS;  /* 饱和，不溢出 */
        }
    }

    /* ---- 心跳 ---- */
    if (++hb_cnt >= VLF_HEARTBEAT_MS) {
        hb_cnt = 0;
        K210Comm_SendFrame("alive");
    }

    /* ---- 电机控制 ---- */
    if (offline) {
        /* 脱线：按上次有效误差方向旋转找回 */
        if (last_valid > 0) {
            pwm_car_rotate_left(VLF_ROTATE_SPEED);
        } else if (last_valid < 0) {
            pwm_car_rotate_right(VLF_ROTATE_SPEED);
        } else {
            pwm_car_forward(VLF_ROTATE_SPEED);
        }
    } else {
        /* 比例差速 */
        int16_t adj = (int16_t)VLF_KP * filtered_err;
        int16_t left_speed;
        int16_t right_speed;

        if (adj >= 0) {
            /* 线偏右 → 左快右慢 */
            left_speed  = clamp(VLF_BASE_SPEED + adj, VLF_MIN_SPEED, VLF_MAX_SPEED);
            right_speed = clamp(VLF_BASE_SPEED - adj, 0, VLF_MAX_SPEED);
        } else {
            /* 线偏左 → 左慢右快 */
            left_speed  = clamp(VLF_BASE_SPEED + adj, 0, VLF_MAX_SPEED);
            right_speed = clamp(VLF_BASE_SPEED - adj, VLF_MIN_SPEED, VLF_MAX_SPEED);
        }

        pwm_motor1_forward(MOTOR1_PWM(left_speed));
        pwm_motor2_forward(MOTOR2_PWM(left_speed));
        pwm_motor3_forward(MOTOR3_PWM(right_speed));
        pwm_motor4_forward(MOTOR4_PWM(right_speed));
    }
}

/* ========== 调试接口 ========== */

int16_t VisionLineFollow_GetFilteredError(void)
{
    return filtered_err;
}

int16_t VisionLineFollow_GetRawError(void)
{
    return raw_error;
}

uint8_t VisionLineFollow_IsOffline(void)
{
    return offline;
}
