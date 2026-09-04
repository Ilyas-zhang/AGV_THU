/*
 * ultrasonic_overtake.c — Ultrasonic obstacle overtaking driver
 *
 * State machine:
 *
 *   FORWARD ──(dist ≤ STOP)──► STOP ──(1s)──► LEFT_SHIFT ──(timed)──►
 *       ▲                                                          |
 *       │                                               PASS ──(clear/timeout)──►
 *       │                                                          |
 *       └──────────────────────────────────── RIGHT_SHIFT ──(timed)─┘
 *
 * dist = 0 (no echo / out of range) → treated as open path.
 *
 * PASS 状态：最少前行 PASS_MIN_MS 后检测距离，
 *           连续 PASS_CLEAR_COUNT 次读数 ≥ SAFE_DIST 才判定超越完成，
 *           超时 PASS_TIMEOUT_MS 兜底。
 */

#include "ultrasonic_overtake.h"
#include "ultrasonic_overtake_config.h"
#include "ultrasonic.h"
#include "motor.h"
#include "led.h"
#include "led_config.h"
#include "buzz.h"

/* ---- state machine ---- */

typedef enum {
    UO_FORWARD,
    UO_STOP,
    UO_LEFT_SHIFT,
    UO_PASS,
    UO_RIGHT_SHIFT
} UO_State;

static UO_State  state        = UO_FORWARD;
static uint16_t  timer        = 0;      /* in-state timer (ms) */
static uint16_t  trigger_cnt  = 0;      /* trigger interval counter */
static uint16_t  last_dist    = 0;      /* last measured distance (mm) */
static uint8_t   clear_count  = 0;      /* consecutive clear readings in PASS */

/* ---- state transition helpers ---- */

static void enter_forward(void)
{
    state = UO_FORWARD;
    pwm_car_forward(UO_FORWARD_SPEED);
    LED_Set(LED_PRESET_FORWARD);
    Buzz_Off();
}

static void enter_stop(void)
{
    state = UO_STOP;
    timer = 0;
    pwm_car_stop();
    LED_Set(LED_PRESET_STOP);
    Buzz_On();
}

static void enter_left_shift(void)
{
    state = UO_LEFT_SHIFT;
    timer = 0;
    pwm_car_turn_left(UO_SHIFT_SPEED);
    LED_Set(LED_PRESET_TURN_LEFT);
    Buzz_Off();
}

static void enter_pass(void)
{
    state = UO_PASS;
    timer = 0;
    clear_count = 0;
    pwm_car_forward(UO_PASS_SPEED);
    LED_Set(LED_PRESET_FORWARD);
    Buzz_Off();
}

static void enter_right_shift(void)
{
    state = UO_RIGHT_SHIFT;
    timer = 0;
    pwm_car_turn_right(UO_SHIFT_SPEED);
    LED_Set(LED_PRESET_TURN_RIGHT);
    Buzz_Off();
}

/* ---- public API ---- */

void UltrasonicOvertake_Init(void)
{
    state       = UO_FORWARD;
    timer       = 0;
    trigger_cnt = 0;
    last_dist   = 0;
    clear_count = 0;

    /* Start moving forward immediately — don't wait for first echo */
    pwm_car_forward(UO_FORWARD_SPEED);
    LED_Set(LED_PRESET_FORWARD);
    Buzz_Off();
}

void UltrasonicOvertake_Tick(void)
{
    /* ---- Periodic ultrasonic trigger ---- */
    if (++trigger_cnt >= UO_TRIGGER_MS) {
        trigger_cnt = 0;
        Ultrasonic_Trigger();
    }

    /* ---- Process new data when available ---- */
    if (Ultrasonic_IsReady()) {
        last_dist = Ultrasonic_GetDistance();  /* mm */

        switch (state) {
        case UO_FORWARD:
            if (last_dist > 0 && last_dist <= UO_STOP_DIST_MM) {
                enter_stop();
            }
            /* else: open path — motor already running */
            break;

        case UO_PASS:
            /* Distance-based pass completion check */
            if (timer >= UO_PASS_MIN_MS) {
                /* dist==0 (no echo) or dist>=SAFE → count as clear */
                if (last_dist == 0 || last_dist >= UO_SAFE_DIST_MM) {
                    if (++clear_count >= UO_PASS_CLEAR_COUNT) {
                        enter_right_shift();
                    }
                } else {
                    clear_count = 0;   /* reset on obstacle reading */
                }
            }
            break;

        case UO_STOP:
        case UO_LEFT_SHIFT:
        case UO_RIGHT_SHIFT:
            /* Timed states: data-driven transitions not needed */
            break;
        }
    }

    /* ---- Timer-driven state transitions (every 1 ms) ---- */
    switch (state) {
    case UO_STOP:
        if (++timer >= UO_STOP_DELAY_MS) {
            enter_left_shift();
        }
        break;

    case UO_LEFT_SHIFT:
        if (++timer >= UO_LEFT_SHIFT_MS) {
            enter_pass();
        }
        break;

    case UO_PASS:
        if (++timer >= UO_PASS_TIMEOUT_MS) {
            /* Safety timeout — force lane return */
            enter_right_shift();
        }
        break;

    case UO_RIGHT_SHIFT:
        if (++timer >= UO_RIGHT_SHIFT_MS) {
            enter_forward();
        }
        break;

    default:
        break;
    }
}

const char *UltrasonicOvertake_GetStateName(void)
{
    static const char *const names[] = {
        "FWD", "STOP", "LSFT", "PASS", "RSFT"
    };
    return names[state];
}

uint16_t UltrasonicOvertake_GetDistance(void)
{
    return last_dist;
}
