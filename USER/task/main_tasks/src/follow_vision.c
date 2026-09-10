/*
 * follow_vision.c — 循迹 + 视觉路牌综合任务
 *
 * 正常循迹行驶 → K210 YOLOv2 路牌识别触发驾驶行为
 *
 * 路牌指令映射 (7 类):
 *   H  (鸣笛)           → 蜂鸣器响 (自动关闭)
 *   L  (左转)           → 延缓直行 → 左旋转入岛 → 岛内循迹 → 左旋转出岛 → 恢复循迹
 *   1  (停车位类型1/PARK1) → 停车
 *   2  (停车位类型2/PARK2) → 停车
 *   R  (右转)           → 延缓直行 → 右旋转入岛 → 岛内循迹 → 右旋转出岛 → 恢复循迹
 *   W  (限速)           → 循迹降速
 *   F  (解除限速)       → 恢复循迹 (速度 FV_LINE_RELEASE_SPEED)
 *
 * 环岛通行不使用 overtake 模块，直接 PWM 控制原地旋转 + 直行。
 * 出岛旋转时传感器初始有信号，需先脱当前线再找新线。
 */

#include "follow_vision.h"
#include "follow_vision_config.h"
#include "irtracking.h"
#include "line_follow.h"
#include "k210_comm.h"
#include "motor.h"
#include "led.h"
#include "led_config.h"
#include "buzz.h"

#if FV_OLED_ENABLE
#include "oled.h"
#endif

/* ---- state machine ---- */

typedef enum {
    FV_FOLLOW,          /* 正常循迹 */
    FV_ISLAND_DELAY,    /* 延缓期：直行接近环岛入口 */
    FV_ISLAND_ROT1,     /* 入岛旋转：原地旋转找环岛黑线（传感器初始无信号） */
    FV_ISLAND_FOLLOW,   /* 岛内循迹：沿环岛黑线行驶 */
    FV_ISLAND_ROT2,     /* 出岛旋转：先脱当前线再找新线（传感器初始有信号） */
    FV_SLOW,            /* 限速循迹 */
    FV_PARK,            /* 停车 (PARK1/PARK2) */
    FV_HORN             /* 鸣笛 (自动关闭) */
} FV_State;

static FV_State  fv_state        = FV_FOLLOW;
static int8_t    fv_dir          = 0;          /* 环岛转向方向: -1=左, +1=右 */
static uint16_t  phase_ms        = 0;          /* 当前阶段计时器 (ms) */
static uint16_t  hb_cnt          = 0;          /* K210 心跳计数 (ms) */
static char      last_sign[3]    = "-";        /* 最近路牌指令 */
static uint16_t  fv_follow_speed = FV_LINE_BASE_SPEED; /* 当前循迹速度 */
static uint8_t   lost_line       = 0;          /* ROT2 子状态: 0=等待脱线, 1=已脱线找新线 */
static FV_State  horn_return     = FV_FOLLOW;  /* HORN 结束后返回的状态 */

/* ---- helpers ---- */

static const char *fv_state_names[] = {
    "FOLLOW", "ISL_DLY", "ISL_R1", "ISL_FLW", "ISL_R2", "SLOW", "PARK", "HORN"
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

/* ---- public API ---- */

void FollowVision_Init(void)
{
    Motor_Init();
    IRTracking_Init();
    K210Comm_Init();
    LineFollow_Init();

    fv_state        = FV_FOLLOW;
    fv_dir          = 0;
    phase_ms        = 0;
    hb_cnt          = 0;
    fv_follow_speed = FV_LINE_BASE_SPEED;
    lost_line       = 0;
    horn_return     = FV_FOLLOW;
    last_sign[0]    = '-';
    last_sign[1]    = '\0';

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
        /* 正常循迹，同时检查 K210 路牌指令 */
        LineFollow_Run(fv_follow_speed);

        if (K210Comm_HasMessage()) {
            const char *msg = K210Comm_GetMessage();
            K210Comm_ClearFlag();

            if (sign_eq(msg, "L")) {
                /* 左转 → 延缓直行 → 左旋转入岛 */
                record_sign("L");
                fv_dir = -1;
                pwm_car_forward(FV_ISLAND_FWD_SPEED);
                fv_state = FV_ISLAND_DELAY;
                phase_ms = 0;

            } else if (sign_eq(msg, "R")) {
                /* 右转 → 延缓直行 → 右旋转入岛 */
                record_sign("R");
                fv_dir = 1;
                pwm_car_forward(FV_ISLAND_FWD_SPEED);
                fv_state = FV_ISLAND_DELAY;
                phase_ms = 0;

            } else if (sign_eq(msg, "H")) {
                /* 鸣笛 → 蜂鸣器响，自动关闭 */
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

    case FV_ISLAND_DELAY:
        /* 延缓期：直行接近环岛入口（pwm_car_forward 已在 FV_FOLLOW 中调用） */
        if (phase_ms >= FV_ISLAND_DELAY_MS) {
            /* 延缓结束 → 原地旋转找环岛黑线 */
            if (fv_dir < 0) {
                /* 左转：左轮后退，右轮前进 → 物理左旋 */
                car_diff_turn(-FV_ISLAND_ROT_SPEED, FV_ISLAND_ROT_SPEED);
            } else {
                /* 右转：左轮前进，右轮后退 → 物理右旋 */
                car_diff_turn(FV_ISLAND_ROT_SPEED, -FV_ISLAND_ROT_SPEED);
            }
            fv_state = FV_ISLAND_ROT1;
            phase_ms = 0;
        }
        break;

    case FV_ISLAND_ROT1:
        /* 入岛旋转：传感器初始无信号，找黑线 */
        if (IRTracking_ReadAll() != 0x0F) {
            /* 找到黑线 → 停止旋转 → 岛内循迹 */
            pwm_car_stop();
            LineFollow_Init();
            fv_state = FV_ISLAND_FOLLOW;
            phase_ms = 0;
        }
        break;

    case FV_ISLAND_FOLLOW:
        /* 岛内循迹：沿环岛黑线行驶 */
        LineFollow_Run(FV_ISLAND_FOLLOW_SPEED);

        if (phase_ms >= FV_ISLAND_RUN_MS) {
            /* 岛内循迹结束 → 原地旋转退出环岛 */
            if (fv_dir < 0) {
                /* 左转：左轮后退，右轮前进 → 物理左旋 */
                car_diff_turn(-FV_ISLAND_ROT_SPEED, FV_ISLAND_ROT_SPEED);
            } else {
                /* 右转：左轮前进，右轮后退 → 物理右旋 */
                car_diff_turn(FV_ISLAND_ROT_SPEED, -FV_ISLAND_ROT_SPEED);
            }
            lost_line = 0;
            fv_state = FV_ISLAND_ROT2;
            phase_ms = 0;
        }
        break;

    case FV_ISLAND_ROT2:
        /* 出岛旋转：先脱当前线再找新线 */
        if (!lost_line) {
            /* 阶段1：等待脱离当前黑线 (所有传感器变白) */
            if (IRTracking_ReadAll() == 0x0F) {
                lost_line = 1;
            }
        } else {
            /* 阶段2：寻找新黑线 */
            if (IRTracking_ReadAll() != 0x0F) {
                /* 找到新黑线 → 停止旋转 → 恢复正常循迹 */
                pwm_car_stop();
                LineFollow_Init();
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
                fv_follow_speed = FV_LINE_RELEASE_SPEED;
                fv_state = FV_FOLLOW;
                phase_ms = 0;

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
                fv_follow_speed = FV_LINE_RELEASE_SPEED;
                LineFollow_Init();
                fv_state = FV_FOLLOW;
                phase_ms = 0;
            }
        }
        break;

    case FV_HORN:
        /* 鸣笛 → 循迹继续，蜂鸣器自动关闭 */
        LineFollow_Run(fv_follow_speed);
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
