/*
 * ultrasonic_overtake.c — Ultrasonic-triggered overtaking driver
 *
 * Thin wrapper around the generic Overtake driver:
 *   - Periodically triggers ultrasonic measurement
 *   - Checks distance to decide when to trigger Overtake_Trigger()
 *   - Drives forward when Overtake is idle
 *   - Forward lock period after overtake completes to avoid re-trigger
 */

#include "ultrasonic_overtake.h"
#include "overtake.h"
#include "ultrasonic.h"
#include "motor.h"
#include "led.h"
#include "led_config.h"

/* ---- private state ---- */

static uint16_t  trigger_cnt  = 0;      /* trigger interval counter */
static uint16_t  last_dist    = 0;      /* last measured distance (mm) */
static uint16_t  fwd_lock     = 0;      /* forward grace period counter (ms) */

/* ---- public API ---- */

void UltrasonicOvertake_Init(void)
{
    Overtake_Init();

    trigger_cnt = 0;
    last_dist   = 0;
    fwd_lock    = UO_FORWARD_LOCK_MS;  /* No lock at startup */

    /* Start moving forward immediately */
    pwm_car_forward(2300);
    LED_Set(LED_PRESET_FORWARD);
}

void UltrasonicOvertake_Tick(void)
{
    /* ---- Drive the overtake state machine ---- */
    Overtake_Tick();

    /* ---- If overtake just completed, resume forward ---- */
    if (Overtake_IsComplete()) {
        fwd_lock = 0;   /* Start forward lock period */
        pwm_car_forward(2300);
        LED_Set(LED_PRESET_FORWARD);
    }

    /* ---- Forward grace period counter ---- */
    if (!Overtake_IsActive() && fwd_lock < UO_FORWARD_LOCK_MS) {
        fwd_lock++;
    }

    /* ---- Periodic ultrasonic trigger ---- */
    if (++trigger_cnt >= UO_TRIGGER_MS) {
        trigger_cnt = 0;
        Ultrasonic_Trigger();
    }

    /* ---- Process new data — only when overtake is idle ---- */
    if (!Overtake_IsActive() && Ultrasonic_IsReady()) {
        last_dist = Ultrasonic_GetDistance();

        /* Only check after grace period expires */
        if (fwd_lock >= UO_FORWARD_LOCK_MS) {
            if (last_dist > 0 && last_dist <= UO_STOP_DIST_MM) {
                Overtake_Trigger(OVERTAKE_DIR_LEFT);
            }
        }
    }
}

const char *UltrasonicOvertake_GetStateName(void)
{
    if (Overtake_IsActive()) {
        return Overtake_GetStateName();
    }
    return "FWD";
}

int8_t UltrasonicOvertake_GetDirection(void)
{
    return Overtake_GetDirection();
}

uint16_t UltrasonicOvertake_GetDistance(void)
{
    return last_dist;
}

uint16_t UltrasonicOvertake_GetTimer(void)
{
    return Overtake_GetTimer();
}
