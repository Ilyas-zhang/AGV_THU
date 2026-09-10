/*
 * vision_drive_v1.c — Vision-driven driving (9-sign version) [LEGACY]
 *
 * ⚠ 旧版驱动，基于 K210 自学习分类器 (9 类)。
 * 新版 YOLOv2 目标检测 (7 类) 驱动见 yolo_drive.c。
 *
 *
 * Receives road sign commands from K210 via USART2 (k210_comm.c ISR):
 *   L (左转)       → 左超车 → 直行 → 右超车 → 恢复
 *   R (右转)       → 右超车 → 直行 → 左超车 → 恢复
 *   H (鸣笛)       → 蜂鸣器响
 *   W (限速)       → 降速行驶
 *   F (解除限速)   → 恢复正常速度
 *   D (红灯)       → 停车
 *   Y (黄灯)       → 停车
 *   G (绿灯)       → 通行/恢复
 *   B (倒车入库)   → 倒车
 *
 * State machine:
 *   VD_IDLE     → waiting for sign, car forward
 *   VD_OVT_1    → first overtake in progress
 *   VD_FWD_WAIT → driving forward between overtakes (timer)
 *   VD_OVT_2    → second overtake in progress (return to lane)
 *   VD_STOPPED  → car stopped (STOP/RED/YELLOW sign)
 *   VD_SLOW     → driving at reduced speed (SLOW sign)
 *   VD_BACKING  → reversing (BACK sign)
 */

#include "vision_drive_v1.h"
#include "overtake.h"
#include "k210_comm.h"
#include "motor.h"
#include "led.h"
#include "led_config.h"
#include "buzz.h"

/* ---- state machine ---- */

typedef enum {
    VD_IDLE,
    VD_OVT_1,
    VD_FWD_WAIT,
    VD_OVT_2,
    VD_STOPPED,
    VD_SLOW,
    VD_BACKING
} VD_State;

static VD_State  vd_state    = VD_IDLE;
static int8_t    vd_dir      = 0;          /* first overtake direction */
static uint16_t  fwd_timer   = 0;         /* forward wait counter (ms) */
static char      last_sign   = '-';       /* last received sign command */

/* ---- helpers ---- */

static const char *vd_state_names[] = {
    "IDLE", "OVT1", "FWD", "OVT2", "STOP", "SLOW", "BACK"
};

static void start_maneuver(int8_t direction)
{
    vd_dir = direction;
    vd_state = VD_OVT_1;
    Overtake_Trigger(direction);
}

/* ---- public API ---- */

void VisionDrive_Init(void)
{
    Overtake_Init();

    vd_state  = VD_IDLE;
    vd_dir    = 0;
    fwd_timer = 0;
    last_sign = '-';

    pwm_car_forward(VD_FORWARD_SPEED);
    LED_Set(LED_PRESET_FORWARD);
    Buzz_Off();
}

void VisionDrive_Tick(void)
{
    /* ---- Always drive the overtake state machine ---- */
    Overtake_Tick();

    /* ---- Vision drive state machine ---- */
    switch (vd_state) {

    case VD_IDLE:
        /* Waiting for sign, car keeps forward — trigger in message handler below */
        break;

    case VD_OVT_1:
        /* First overtake in progress */
        if (Overtake_IsComplete()) {
            pwm_car_forward(VD_FORWARD_SPEED);
            LED_Set(LED_PRESET_FORWARD);
            Buzz_Off();
            fwd_timer = 0;
            vd_state = VD_FWD_WAIT;
        }
        break;

    case VD_FWD_WAIT:
        /* Driving forward between overtakes */
        if (++fwd_timer >= VD_FWD_WAIT_MS) {
            int8_t reverse_dir = (vd_dir == OVERTAKE_DIR_LEFT)
                                 ? OVERTAKE_DIR_RIGHT
                                 : OVERTAKE_DIR_LEFT;
            Overtake_Trigger(reverse_dir);
            vd_state = VD_OVT_2;
        }
        break;

    case VD_OVT_2:
        /* Second overtake in progress (return to original lane) */
        if (Overtake_IsComplete()) {
            pwm_car_forward(VD_FORWARD_SPEED);
            LED_Set(LED_PRESET_FORWARD);
            Buzz_Off();
            vd_state = VD_IDLE;
        }
        break;

    case VD_STOPPED:
        /* Car stopped — can re-trigger from message handler below */
        break;

    case VD_SLOW:
        /* Driving at reduced speed — can re-trigger from message handler */
        break;

    case VD_BACKING:
        /* Reversing — can re-trigger from message handler */
        break;
    }

    /* ---- Process K210 sign commands ---- */
    if (K210Comm_HasMessage()) {
        const char *msg = K210Comm_GetMessage();
        K210Comm_ClearFlag();

        char cmd = msg[0];

        /* Record last sign for display */
        if (cmd == 'L' || cmd == 'R' || cmd == 'S' || cmd == 'H'
            || cmd == 'W' || cmd == 'F' || cmd == 'D' || cmd == 'Y'
            || cmd == 'G' || cmd == 'B' || cmd == 'a') {
            last_sign = cmd;
        }

        /* Only accept new maneuver when not in overtake sequence */
        if (vd_state == VD_IDLE || vd_state == VD_STOPPED
            || vd_state == VD_SLOW || vd_state == VD_BACKING) {

            switch (cmd) {
            case 'L':
                /* 左转 → 左超车 */
                start_maneuver(OVERTAKE_DIR_LEFT);
                break;

            case 'R':
                /* 右转 → 右超车 */
                start_maneuver(OVERTAKE_DIR_RIGHT);
                break;

            case 'S':
            case 'D':
            case 'Y':
                /* STOP / 红灯 / 黄灯 → 停车 */
                pwm_car_stop();
                LED_Set(LED_PRESET_STOP);
                Buzz_On();
                vd_state = VD_STOPPED;
                break;

            case 'H':
                /* 鸣笛 */
                Buzz_On();
                break;

            case 'W':
                /* 限速 → 降速行驶 */
                pwm_car_forward(VD_SLOW_SPEED);
                LED_Set(LED_PRESET_FORWARD);
                vd_state = VD_SLOW;
                break;

            case 'F':
            case 'G':
                /* 解除限速 / 绿灯 → 恢复正常速度 */
                pwm_car_forward(VD_FORWARD_SPEED);
                LED_Set(LED_PRESET_FORWARD);
                Buzz_Off();
                vd_state = VD_IDLE;
                break;

            case 'B':
                /* 倒车入库 */
                pwm_car_backward(VD_BACK_SPEED);
                LED_Set(LED_PRESET_STOP);
                vd_state = VD_BACKING;
                break;

            default:
                break;
            }
        }
    }
}

const char *VisionDrive_GetStateName(void)
{
    return vd_state_names[vd_state];
}

const char *VisionDrive_GetDetailStateName(void)
{
    if (Overtake_IsActive()) {
        return Overtake_GetStateName();
    }
    return vd_state_names[vd_state];
}

char VisionDrive_GetLastSign(void)
{
    return last_sign;
}

uint8_t VisionDrive_IsActive(void)
{
    return (vd_state != VD_IDLE && vd_state != VD_STOPPED) ? 1 : 0;
}
