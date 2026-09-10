/*
 * follow_vision.c — 循迹 + 视觉路牌综合任务
 *
 * 正常循迹行驶 → K210 YOLOv2 路牌识别触发驾驶行为
 *
 * 路牌指令映射 (7 类):
 *   H  (鸣笛)           → 蜂鸣器响 (自动关闭)
 *   L  (左转)           → 左超车 → 直行 → 右超车 → 恢复循迹
 *   P1 (停车位类型1)    → 停车
 *   P2 (停车位类型2)    → 停车
 *   R  (右转)           → 右超车 → 直行 → 左超车 → 恢复循迹
 *   W  (限速)           → 循迹降速
 *   F  (解除限速)       → 恢复正常循迹速度
 *
 * 超车期间不调用 LineFollow_Run。
 * 超车完成后检查黑线 → 找到直接循迹 / 脱线则旋转搜索。
 */

#include "follow_vision.h"
#include "follow_vision_config.h"
#include "irtracking.h"
#include "line_follow.h"
#include "k210_comm.h"
#include "motor.h"
#include "overtake.h"
#include "led.h"
#include "led_config.h"
#include "buzz.h"

#if FV_OLED_ENABLE
#include "oled.h"
#endif

/* ---- state machine ---- */

typedef enum {
    FV_FOLLOW,      /* 正常循迹 */
    FV_OVT_1,       /* 第一次超车进行中 */
    FV_FWD_WAIT,    /* 两次超车间直行 */
    FV_OVT_2,       /* 第二次超车进行中 */
    FV_SEARCH,      /* 脱线搜索：原地旋转找黑线 */
    FV_SLOW,        /* 限速循迹 */
    FV_PARK,        /* 停车 (PARK1/PARK2) */
    FV_HORN         /* 鸣笛 (自动关闭) */
} FV_State;

static FV_State  fv_state    = FV_FOLLOW;
static int8_t    fv_dir      = 0;          /* 第一次超车方向 */
static uint16_t  phase_ms    = 0;          /* 当前阶段计时器 (ms) */
static uint16_t  hb_cnt      = 0;          /* K210 心跳计数 (ms) */
static char      last_sign[3] = "-";       /* 最近路牌指令 */

/* ---- helpers ---- */

static const char *fv_state_names[] = {
    "FOLLOW", "OVT1", "FWD", "OVT2", "SEARCH", "SLOW", "PARK", "HORN"
};

/** 用第一次超车参数覆盖 overtake 默认值 */
static void fv_set_ovt1_config(void)
{
    Overtake_Config oc;
    oc.stop_delay_ms = FV_OVT1_STOP_DELAY_MS;
    oc.rotate_ms     = FV_OVT1_ROTATE_MS;
    oc.pass_ms       = FV_OVT1_PASS_MS;
    oc.rotate_speed  = FV_OVT1_ROTATE_SPEED;
    oc.pass_speed    = FV_OVT1_PASS_SPEED;
    Overtake_SetConfig(&oc);
}

/** 用第二次超车参数覆盖 overtake 默认值 */
static void fv_set_ovt2_config(void)
{
    Overtake_Config oc;
    oc.stop_delay_ms = FV_OVT2_STOP_DELAY_MS;
    oc.rotate_ms     = FV_OVT2_ROTATE_MS;
    oc.pass_ms       = FV_OVT2_PASS_MS;
    oc.rotate_speed  = FV_OVT2_ROTATE_SPEED;
    oc.pass_speed    = FV_OVT2_PASS_SPEED;
    Overtake_SetConfig(&oc);
}

/** Match sign payload — supports single-char (H/L/R/W/F) and multi-char (P1/P2) */
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
    Overtake_Init();

    fv_state   = FV_FOLLOW;
    fv_dir     = 0;
    phase_ms   = 0;
    hb_cnt     = 0;
    last_sign[0] = '-';
    last_sign[1] = '\0';

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

    /* ---- 超车驱动 tick（超车进行中才推进） ---- */
    if (fv_state == FV_OVT_1 || fv_state == FV_OVT_2) {
        Overtake_Tick();
    }

    phase_ms++;

    /* ---- 状态机 ---- */
    switch (fv_state) {

    case FV_FOLLOW:
        /* 正常循迹，同时检查 K210 路牌指令 */
        LineFollow_Run(FV_LINE_BASE_SPEED);

        if (K210Comm_HasMessage()) {
            const char *msg = K210Comm_GetMessage();
            K210Comm_ClearFlag();

            if (sign_eq(msg, "L")) {
                /* 左转 → 左超车 */
                record_sign("L");
                Overtake_Init();
                fv_set_ovt1_config();
                Overtake_Trigger(OVERTAKE_DIR_LEFT);
                fv_dir = OVERTAKE_DIR_LEFT;
                fv_state = FV_OVT_1;
                phase_ms = 0;

            } else if (sign_eq(msg, "R")) {
                /* 右转 → 右超车 */
                record_sign("R");
                Overtake_Init();
                fv_set_ovt1_config();
                Overtake_Trigger(OVERTAKE_DIR_RIGHT);
                fv_dir = OVERTAKE_DIR_RIGHT;
                fv_state = FV_OVT_1;
                phase_ms = 0;

            } else if (sign_eq(msg, "H")) {
                /* 鸣笛 → 蜂鸣器响，自动关闭 */
                record_sign("H");
                Buzz_On();
                fv_state = FV_HORN;
                phase_ms = 0;

            } else if (sign_eq(msg, "W")) {
                /* 限速 → 循迹降速 */
                record_sign("W");
                fv_state = FV_SLOW;
                phase_ms = 0;

            } else if (sign_eq(msg, "P1") || sign_eq(msg, "P2")) {
                /* 停车 (PARK1/PARK2) */
                record_sign(msg);
                pwm_car_stop();
                LED_Set(LED_PRESET_STOP);
                Buzz_On();
                fv_state = FV_PARK;
                phase_ms = 0;
            }
        }
        break;

    case FV_OVT_1:
        /* 第一次超车进行中 → 完成后直行超过障碍物 */
        if (Overtake_IsComplete()) {
            pwm_car_forward(FV_FWD_WAIT_SPEED);
            LED_Set(LED_PRESET_FORWARD);
            Buzz_Off();
            fv_state = FV_FWD_WAIT;
            phase_ms = 0;
        }
        break;

    case FV_FWD_WAIT:
        /* 两次超车间直行 → 触发反向超车 */
        if (phase_ms >= FV_FWD_WAIT_MS) {
            int8_t reverse_dir = (fv_dir == OVERTAKE_DIR_LEFT)
                                 ? OVERTAKE_DIR_RIGHT
                                 : OVERTAKE_DIR_LEFT;
            Overtake_Init();
            fv_set_ovt2_config();
            Overtake_Trigger(reverse_dir);
            fv_state = FV_OVT_2;
            phase_ms = 0;
        }
        break;

    case FV_OVT_2:
        /* 第二次超车进行中 → 完成后检查黑线 */
        if (Overtake_IsComplete()) {
            /* 任意传感器检测到黑线 (0=黑线) → 直接恢复循迹 */
            if (IRTracking_ReadAll() != 0x0F) {
                LineFollow_Init();
                fv_state = FV_FOLLOW;
                phase_ms = 0;
            } else {
                /* 脱线：原地旋转找黑线 */
                pwm_car_rotate_left(FV_SEARCH_SPEED);  /* 物理接线反向：代码 left = 实际右旋 */
                LED_Set(LED_PRESET_ROTATE);
                fv_state = FV_SEARCH;
                phase_ms = 0;
            }
        }
        break;

    case FV_SEARCH:
        /* 原地旋转找黑线 → 找到或超时后恢复循迹 */
        if (IRTracking_ReadAll() != 0x0F) {
            /* 找到黑线 → 恢复循迹 */
            pwm_car_stop();
            LineFollow_Init();
            fv_state = FV_FOLLOW;
            phase_ms = 0;
        } else if (phase_ms >= FV_SEARCH_TIMEOUT_MS) {
            /* 超时 → 停车，交由循迹自身脱线恢复处理 */
            pwm_car_stop();
            LineFollow_Init();
            fv_state = FV_FOLLOW;
            phase_ms = 0;
        }
        break;

    case FV_SLOW:
        /* 限速循迹 → 检查 K210 指令 */
        LineFollow_Run(FV_LINE_SLOW_SPEED);

        if (K210Comm_HasMessage()) {
            const char *msg = K210Comm_GetMessage();
            K210Comm_ClearFlag();

            if (sign_eq(msg, "F")) {
                /* 解除限速 → 恢复正常循迹 */
                record_sign("F");
                Buzz_Off();
                fv_state = FV_FOLLOW;
                phase_ms = 0;

            } else if (sign_eq(msg, "P1") || sign_eq(msg, "P2")) {
                /* 停车 */
                record_sign(msg);
                pwm_car_stop();
                LED_Set(LED_PRESET_STOP);
                Buzz_On();
                fv_state = FV_PARK;
                phase_ms = 0;

            } else if (sign_eq(msg, "H")) {
                /* 鸣笛 */
                record_sign("H");
                Buzz_On();
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
                LineFollow_Init();
                fv_state = FV_FOLLOW;
                phase_ms = 0;
            }
        }
        break;

    case FV_HORN:
        /* 鸣笛 → 自动关闭 */
        if (phase_ms >= FV_HORN_MS) {
            Buzz_Off();
            fv_state = FV_FOLLOW;
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
