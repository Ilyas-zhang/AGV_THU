/*
 * yolo_drive.c — YOLOv2 vision-driven driving (7-sign version)
 *
 * Receives road sign commands from K210 via USART2 (k210_comm.c ISR):
 *   H  (鸣笛)           → 蜂鸣器响 (自动关闭)
 *   L  (左转)           → 左超车 → 直行 → 右超车 → 恢复
 *   P1 (停车位类型1)    → 停车
 *   P2 (停车位类型2)    → 停车
 *   R  (右转)           → 右超车 → 直行 → 左超车 → 恢复
 *   W  (限速)           → 降速行驶
 *   F  (解除限速)       → 恢复正常速度
 *
 * State machine:
 *   YD_IDLE     → waiting for sign, car forward
 *   YD_OVT_1    → first overtake in progress
 *   YD_FWD_WAIT → driving forward between overtakes (timer)
 *   YD_OVT_2    → second overtake in progress (return to lane)
 *   YD_STOPPED  → car stopped (PARK1/PARK2 sign)
 *   YD_SLOW     → driving at reduced speed (SPEED_LIMIT sign)
 *   YD_HORN     → buzzer on with auto-off timer
 */

#include "yolo_drive.h"
#include "overtake.h"
#include "k210_comm.h"
#include "motor.h"
#include "led.h"
#include "led_config.h"
#include "buzz.h"

/* ---- state machine ---- */

typedef enum {
    YD_IDLE,
    YD_OVT_1,
    YD_FWD_WAIT,
    YD_OVT_2,
    YD_STOPPED,
    YD_SLOW,
    YD_HORN
} YD_State;

static YD_State  yd_state    = YD_IDLE;
static int8_t    yd_dir      = 0;          /* first overtake direction */
static uint16_t  fwd_timer   = 0;         /* forward wait counter (ms) */
static uint16_t  horn_timer  = 0;         /* horn auto-off counter (ms) */
static char      last_sign[3] = "-";      /* last received sign payload */

/* ---- helpers ---- */

static const char *yd_state_names[] = {
    "IDLE", "OVT1", "FWD", "OVT2", "STOP", "SLOW", "HORN"
};

static void start_maneuver(int8_t direction)
{
    yd_dir = direction;
    yd_state = YD_OVT_1;
    Overtake_Trigger(direction);
}

/** Match sign payload — supports single-char (H/L/R/W/F) and multi-char (P1/P2) */
static int sign_eq(const char *msg, const char *sign)
{
    /* Compare up to sign length */
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

void YoloDrive_Init(void)
{
    Overtake_Init();

    yd_state   = YD_IDLE;
    yd_dir     = 0;
    fwd_timer  = 0;
    horn_timer = 0;
    last_sign[0] = '-';
    last_sign[1] = '\0';

    pwm_car_forward(YD_FORWARD_SPEED);
    LED_Set(LED_PRESET_FORWARD);
    Buzz_Off();
}

void YoloDrive_Tick(void)
{
    /* ---- Always drive the overtake state machine ---- */
    Overtake_Tick();

    /* ---- Horn auto-off timer ---- */
    if (yd_state == YD_HORN) {
        if (++horn_timer >= YD_HORN_MS) {
            Buzz_Off();
            pwm_car_forward(YD_FORWARD_SPEED);
            LED_Set(LED_PRESET_FORWARD);
            yd_state = YD_IDLE;
        }
    }

    /* ---- Yolo drive state machine ---- */
    switch (yd_state) {

    case YD_IDLE:
        /* Waiting for sign, car keeps forward — trigger in message handler below */
        break;

    case YD_OVT_1:
        /* First overtake in progress */
        if (Overtake_IsComplete()) {
            pwm_car_forward(YD_FORWARD_SPEED);
            LED_Set(LED_PRESET_FORWARD);
            Buzz_Off();
            fwd_timer = 0;
            yd_state = YD_FWD_WAIT;
        }
        break;

    case YD_FWD_WAIT:
        /* Driving forward between overtakes */
        if (++fwd_timer >= YD_FWD_WAIT_MS) {
            int8_t reverse_dir = (yd_dir == OVERTAKE_DIR_LEFT)
                                 ? OVERTAKE_DIR_RIGHT
                                 : OVERTAKE_DIR_LEFT;
            Overtake_Trigger(reverse_dir);
            yd_state = YD_OVT_2;
        }
        break;

    case YD_OVT_2:
        /* Second overtake in progress (return to original lane) */
        if (Overtake_IsComplete()) {
            pwm_car_forward(YD_FORWARD_SPEED);
            LED_Set(LED_PRESET_FORWARD);
            Buzz_Off();
            yd_state = YD_IDLE;
        }
        break;

    case YD_STOPPED:
        /* Car stopped — can re-trigger from message handler below */
        break;

    case YD_SLOW:
        /* Driving at reduced speed — can re-trigger from message handler */
        break;

    default:
        break;
    }

    /* ---- Process K210 sign commands ---- */
    if (K210Comm_HasMessage()) {
        const char *msg = K210Comm_GetMessage();
        K210Comm_ClearFlag();

        /* Only accept new maneuver when not in overtake sequence or horn */
        if (yd_state == YD_IDLE || yd_state == YD_STOPPED
            || yd_state == YD_SLOW) {

            if (sign_eq(msg, "L")) {
                /* 左转 → 左超车 */
                record_sign("L");
                start_maneuver(OVERTAKE_DIR_LEFT);

            } else if (sign_eq(msg, "R")) {
                /* 右转 → 右超车 */
                record_sign("R");
                start_maneuver(OVERTAKE_DIR_RIGHT);

            } else if (sign_eq(msg, "P1") || sign_eq(msg, "P2")) {
                /* 停车 (PARK1 / PARK2) → 停车 */
                record_sign(msg);
                pwm_car_stop();
                LED_Set(LED_PRESET_STOP);
                Buzz_On();
                yd_state = YD_STOPPED;

            } else if (sign_eq(msg, "H")) {
                /* 鸣笛 → 蜂鸣器响，自动关闭 */
                record_sign("H");
                Buzz_On();
                horn_timer = 0;
                yd_state = YD_HORN;

            } else if (sign_eq(msg, "W")) {
                /* 限速 → 降速行驶 */
                record_sign("W");
                pwm_car_forward(YD_SLOW_SPEED);
                LED_Set(LED_PRESET_FORWARD);
                yd_state = YD_SLOW;

            } else if (sign_eq(msg, "F")) {
                /* 解除限速 → 恢复正常速度 */
                record_sign("F");
                pwm_car_forward(YD_FORWARD_SPEED);
                LED_Set(LED_PRESET_FORWARD);
                Buzz_Off();
                yd_state = YD_IDLE;
            }
        }
    }
}

const char *YoloDrive_GetStateName(void)
{
    return yd_state_names[yd_state];
}

const char *YoloDrive_GetDetailStateName(void)
{
    if (Overtake_IsActive()) {
        return Overtake_GetStateName();
    }
    return yd_state_names[yd_state];
}

const char *YoloDrive_GetLastSign(void)
{
    return last_sign;
}

uint8_t YoloDrive_IsActive(void)
{
    return (yd_state != YD_IDLE && yd_state != YD_STOPPED) ? 1 : 0;
}

uint8_t YoloDrive_IsComplete(void)
{
    return (yd_state == YD_IDLE || yd_state == YD_STOPPED || yd_state == YD_SLOW) ? 1 : 0;
}
