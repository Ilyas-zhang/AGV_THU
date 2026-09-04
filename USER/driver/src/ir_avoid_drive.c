/*
 * ir_avoid_drive.c — IR obstacle avoidance driving driver
 *
 * State machine:
 *
 *              ┌─(仅左障碍)──► TURN_RIGHT ──(timed)──┐
 *   FORWARD ───┤─(仅右障碍)──► TURN_LEFT  ──(timed)──┤──► FORWARD
 *              └─(双侧障碍)──► BACKUP ──(timed)──► ROTATE ──(timed)──┘
 *
 *   (无障碍) ──► 保持 FORWARD
 *
 * 传感器：IRAvoid_ReadAll() 返回 bit0=左, bit1=右
 * 去抖滤波由 IRAvoid_Tick() (1 kHz) 处理，本驱动只读滤波后结果。
 */

#include "ir_avoid_drive.h"
#include "ir_avoid_drive_config.h"
#include "ir_avoid.h"
#include "motor.h"
#include "led.h"
#include "led_config.h"
#include "buzz.h"

/* ---- state machine ---- */

typedef enum {
    IAD_FORWARD,
    IAD_TURN_RIGHT,
    IAD_TURN_LEFT,
    IAD_BACKUP,
    IAD_ROTATE
} IAD_State;

static IAD_State  state = IAD_FORWARD;
static uint16_t   timer = 0;      /* in-state timer (ms) */

/* ---- state transition helpers ---- */

static void enter_forward(void)
{
    state = IAD_FORWARD;
    pwm_car_forward(IAD_FORWARD_SPEED);
    LED_Set(LED_PRESET_FORWARD);
    Buzz_Off();
}

static void enter_turn_right(void)
{
    state = IAD_TURN_RIGHT;
    timer = 0;
    pwm_car_turn_right(IAD_TURN_SPEED);
    LED_Set(LED_PRESET_TURN_RIGHT);
    Buzz_Off();
}

static void enter_turn_left(void)
{
    state = IAD_TURN_LEFT;
    timer = 0;
    pwm_car_turn_left(IAD_TURN_SPEED);
    LED_Set(LED_PRESET_TURN_LEFT);
    Buzz_Off();
}

static void enter_backup(void)
{
    state = IAD_BACKUP;
    timer = 0;
    pwm_car_backward(IAD_BACKUP_SPEED);
    LED_Set(LED_PRESET_BACKWARD);
    Buzz_On();
}

static void enter_rotate(void)
{
    state = IAD_ROTATE;
    timer = 0;
#if IAD_ROTATE_DIR > 0
    pwm_car_rotate_right(IAD_ROTATE_SPEED);
#else
    pwm_car_rotate_left(IAD_ROTATE_SPEED);
#endif
    LED_Set(LED_PRESET_ROTATE);
    Buzz_Off();
}

/* ---- public API ---- */

void IRAvoidDrive_Init(void)
{
    state = IAD_FORWARD;
    timer = 0;

    /* Start moving forward immediately */
    pwm_car_forward(IAD_FORWARD_SPEED);
    LED_Set(LED_PRESET_FORWARD);
    Buzz_Off();
}

void IRAvoidDrive_Tick(void)
{
    /* ---- Sensor-driven transitions (only in FORWARD) ---- */
    if (state == IAD_FORWARD) {
        uint8_t sensors = IRAvoid_ReadAll();  /* bit0=left, bit1=right */

        if (sensors == 0x03) {
            /* Both obstacles → back up then rotate */
            enter_backup();
        } else if (sensors == 0x01) {
            /* Left obstacle only → turn right */
            enter_turn_right();
        } else if (sensors == 0x02) {
            /* Right obstacle only → turn left */
            enter_turn_left();
        }
        /* else: no obstacle — stay FORWARD */
    }

    /* ---- Timer-driven state transitions (every 1 ms) ---- */
    switch (state) {
    case IAD_TURN_RIGHT:
        if (++timer >= IAD_TURN_MS) {
            enter_forward();
        }
        break;

    case IAD_TURN_LEFT:
        if (++timer >= IAD_TURN_MS) {
            enter_forward();
        }
        break;

    case IAD_BACKUP:
        if (++timer >= IAD_BACKUP_MS) {
            enter_rotate();
        }
        break;

    case IAD_ROTATE:
        if (++timer >= IAD_ROTATE_MS) {
            enter_forward();
        }
        break;

    default:
        break;
    }
}

const char *IRAvoidDrive_GetStateName(void)
{
    static const char *const names[] = {
        "FWD", "TR", "TL", "BACK", "ROT"
    };
    return names[state];
}
