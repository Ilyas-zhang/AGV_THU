/*
 * follow_avoid.c — 循迹 + 超声避障综合任务
 *
 * 逻辑：
 *   正常循迹 → 超声检测到障碍物
 *   → 停车 0.5s（循迹暂停）
 *   → 右超车（委托 overtake.c）
 *   → 直行超过障碍物 2s
 *   → 左超车（委托 overtake.c）
 *   → 恢复循迹
 *
 * 避障期间不调用 LineFollow_Run。
 */

#include "follow_avoid.h"
#include "follow_avoid_config.h"
#include "irtracking.h"
#include "line_follow.h"
#include "ultrasonic.h"
#include "motor.h"
#include "overtake.h"
#include "led.h"
#include "led_config.h"
#include "buzz.h"

#if FA_OLED_ENABLE
#include "oled.h"
#endif

/* ---- state machine ---- */

typedef enum {
    FA_FOLLOW,      /* 正常循迹 */
    FA_STOP,        /* 停车等待 */
    FA_OVT_1,       /* 右超车进行中 */
    FA_WAIT,        /* 两次超车间直行 */
    FA_OVT_2,       /* 左超车进行中 */
    FA_SEARCH       /* 脱线搜索：原地右旋找黑线 */
} FA_State;

static FA_State  fa_state    = FA_FOLLOW;
static uint16_t  phase_ms    = 0;      /* 当前阶段计时器 (ms) */
static uint16_t  last_dist   = 9999;   /* 最近一次超声距离 (mm) */
static uint16_t  trigger_cnt = 0;      /* 超声触发间隔计数器 */

/* ---- helpers ---- */

static const char *fa_state_names[] = {
    "FOLLOW", "STOP", "OVT1", "FWD", "OVT2", "SEARCH"
};

/** 用第一次超车参数覆盖 overtake 默认值 */
static void fa_set_ovt1_config(void)
{
    Overtake_Config oc;
    oc.stop_delay_ms = FA_OVT1_STOP_DELAY_MS;
    oc.rotate_ms     = FA_OVT1_ROTATE_MS;
    oc.pass_ms       = FA_OVT1_PASS_MS;
    oc.rotate_speed  = FA_OVT1_ROTATE_SPEED;
    oc.pass_speed    = FA_OVT1_PASS_SPEED;
    Overtake_SetConfig(&oc);
}

/** 用第二次超车参数覆盖 overtake 默认值 */
static void fa_set_ovt2_config(void)
{
    Overtake_Config oc;
    oc.stop_delay_ms = FA_OVT2_STOP_DELAY_MS;
    oc.rotate_ms     = FA_OVT2_ROTATE_MS;
    oc.pass_ms       = FA_OVT2_PASS_MS;
    oc.rotate_speed  = FA_OVT2_ROTATE_SPEED;
    oc.pass_speed    = FA_OVT2_PASS_SPEED;
    Overtake_SetConfig(&oc);
}

/* ---- public API ---- */

void FollowAvoid_Init(void)
{
    Motor_Init();
    IRTracking_Init();
    Ultrasonic_Init();
    LineFollow_Init();
    Overtake_Init();
    fa_set_ovt1_config();

    fa_state    = FA_FOLLOW;
    phase_ms    = 0;
    last_dist   = 9999;
    trigger_cnt = 0;

#if FA_OLED_ENABLE
    OLED_Init();
    OLED_Clear();
    OLED_GotoXY(0, 0);
    OLED_Puts("FA: FOLLOW", &Font_7x10, OLED_COLOR_WHITE);
    OLED_GotoXY(0, 10);
    OLED_Puts("D:----mm", &Font_7x10, OLED_COLOR_WHITE);
    OLED_Update();
#endif
}

void FollowAvoid_Tick(void)
{
    /* ---- 周期触发超声测距 ---- */
    if (++trigger_cnt >= FA_ULTRASONIC_TRIGGER_MS) {
        trigger_cnt = 0;
        Ultrasonic_Trigger();
    }

    /* ---- 读取超声距离 ---- */
    uint16_t dist = Ultrasonic_GetDistance();
    if (dist > 0) last_dist = dist;

    /* ---- 超车驱动 tick（超车进行中才推进） ---- */
    if (fa_state == FA_OVT_1 || fa_state == FA_OVT_2) {
        Overtake_Tick();
    }

    phase_ms++;

    /* ---- 状态机 ---- */
    switch (fa_state) {

    case FA_FOLLOW:
        /* 正常循迹，超声检测障碍物 */
        if (last_dist > 0 && last_dist <= FA_STOP_DIST) {
            pwm_car_stop();
            fa_state = FA_STOP;
            phase_ms = 0;
        } else if (last_dist > 0 && last_dist <= FA_WARN_DIST) {
            LineFollow_Run(FA_LINE_SLOW_SPEED);
        } else {
            LineFollow_Run(FA_LINE_BASE_SPEED);
        }
        break;

    case FA_STOP:
        /* 停车等待 → 触发右超车 */
        if (phase_ms >= FA_STOP_DELAY_MS) {
            Overtake_Init();
            fa_set_ovt1_config();
            Overtake_Trigger(FA_OVT1_DIR);
            fa_state = FA_OVT_1;
            phase_ms = 0;
        }
        break;

    case FA_OVT_1:
        /* 右超车进行中 → 完成后直行超过障碍物 */
        if (Overtake_IsComplete()) {
            pwm_car_forward(FA_FWD_WAIT_SPEED);
            LED_Set(LED_PRESET_FORWARD);
            Buzz_Off();
            fa_state = FA_WAIT;
            phase_ms = 0;
        }
        break;

    case FA_WAIT:
        /* 两次超车间直行 → 触发左超车 */
        if (phase_ms >= FA_OVT_GAP_MS) {
            Overtake_Init();
            fa_set_ovt2_config();
            Overtake_Trigger(FA_OVT2_DIR);
            fa_state = FA_OVT_2;
            phase_ms = 0;
        }
        break;

    case FA_OVT_2:
        /* 左超车进行中 → 完成后检查黑线 */
        if (Overtake_IsComplete()) {
            /* 任意传感器检测到黑线 (0=黑线) → 直接恢复循迹 */
            if (IRTracking_ReadAll() != 0x0F) {
                LineFollow_Init();
                fa_state = FA_FOLLOW;
                phase_ms = 0;
            } else {
                /* 脱线：原地右旋找黑线 */
                pwm_car_rotate_left(FA_SEARCH_SPEED);  /* 物理接线反向：代码 left = 实际右旋 */
                LED_Set(LED_PRESET_ROTATE);
                fa_state = FA_SEARCH;
                phase_ms = 0;
            }
        }
        break;

    case FA_SEARCH:
        /* 原地右旋找黑线 → 找到或超时后恢复循迹 */
        if (IRTracking_ReadAll() != 0x0F) {
            /* 找到黑线 → 恢复循迹 */
            pwm_car_stop();
            LineFollow_Init();
            fa_state = FA_FOLLOW;
            phase_ms = 0;
        } else if (phase_ms >= FA_SEARCH_TIMEOUT_MS) {
            /* 超时 → 停车，交由循迹自身脱线恢复处理 */
            pwm_car_stop();
            LineFollow_Init();
            fa_state = FA_FOLLOW;
            phase_ms = 0;
        }
        break;
    }

    /* ---- OLED 显示 ---- */
#if FA_OLED_ENABLE
    {
        static uint16_t disp_cnt = 0;
        if (++disp_cnt < FA_DISPLAY_MS) return;
        disp_cnt = 0;

        /* Distance as 4-digit string */
        char dist_str[9];
        uint16_t d = last_dist;
        dist_str[0] = 'D'; dist_str[1] = ':';
        dist_str[2] = '0' + (d / 1000) % 10;
        dist_str[3] = '0' + (d / 100) % 10;
        dist_str[4] = '0' + (d / 10) % 10;
        dist_str[5] = '0' + d % 10;
        dist_str[6] = 'm'; dist_str[7] = 'm'; dist_str[8] = '\0';

        OLED_Clear();
        OLED_GotoXY(0, 0);
        OLED_Puts("FA:", &Font_7x10, OLED_COLOR_WHITE);
        OLED_Puts(fa_state_names[fa_state], &Font_7x10, OLED_COLOR_WHITE);
        OLED_GotoXY(0, 10);
        OLED_Puts(dist_str, &Font_7x10, OLED_COLOR_WHITE);
        OLED_Update();
    }
#endif
}

const char *FollowAvoid_GetStateName(void)
{
    return fa_state_names[fa_state];
}

uint16_t FollowAvoid_GetDistance(void)
{
    return last_dist;
}
