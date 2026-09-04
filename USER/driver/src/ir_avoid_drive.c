/*
 * ir_avoid_drive.c — IR obstacle avoidance driving driver
 *
 * Uses Overtake driver for avoidance maneuvers:
 *   Left obstacle  → right overtake
 *   Right obstacle → left overtake
 *   Both obstacles → left overtake
 *   No obstacle    → forward
 */

#include "ir_avoid_drive.h"
#include "overtake.h"
#include "ir_avoid.h"
#include "motor.h"
#include "led.h"
#include "led_config.h"
#include "buzz.h"

/* ---- public API ---- */

void IRAvoidDrive_Init(void)
{
    Overtake_Init();
    pwm_car_forward(IAD_FORWARD_SPEED);
    LED_Set(LED_PRESET_FORWARD);
    Buzz_Off();
}

void IRAvoidDrive_Tick(void)
{
    /* ---- Always drive the overtake state machine ---- */
    Overtake_Tick();

    /* ---- If overtake just completed, resume forward ---- */
    if (Overtake_IsComplete()) {
        pwm_car_forward(IAD_FORWARD_SPEED);
        LED_Set(LED_PRESET_FORWARD);
        Buzz_Off();
    }

    /* ---- If overtake is active, just wait ---- */
    if (Overtake_IsActive()) {
        return;
    }

    /* ---- Check sensors and trigger overtake ---- */
    uint8_t sensors = IRAvoid_ReadAll();

    if (sensors == 0x03) {
        /* Both obstacles → left overtake */
        Overtake_Trigger(IAD_BOTH_OVERTAKE_DIR);
    } else if (sensors == 0x01) {
        /* Left obstacle → right overtake */
        Overtake_Trigger(OVERTAKE_DIR_RIGHT);
    } else if (sensors == 0x02) {
        /* Right obstacle → left overtake */
        Overtake_Trigger(OVERTAKE_DIR_LEFT);
    }
}

const char *IRAvoidDrive_GetStateName(void)
{
    if (Overtake_IsActive()) {
        return Overtake_GetStateName();
    }
    return "FWD";
}
