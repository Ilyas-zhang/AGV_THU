/*
 * obstacle_avoid.c — Ultrasonic obstacle avoidance driver
 *
 * State machine:
 *
 *   FORWARD ──(dist ≤ WARN)──► SLOW ──(dist ≤ STOP)──► STOP
 *      ▲                         │                        │
 *      │                         │(dist ≥ SAFE)           │ 停留 500 ms
 *      └─────────────────────────┘                        ▼
 *                                                 BACKUP (800 ms)
 *                                                      │
 *                                                      ▼
 *                                                 TURN   (600 ms)
 *                                                      │
 *                                                      ▼
 *                                                    FORWARD
 *
 * dist = 0 (no echo / out of range) → treated as open path, stay FORWARD.
 */

#include "obstacle_avoid.h"
#include "ultrasonic.h"
#include "motor.h"
#include "motor_config.h"
#include "led.h"
#include "led_config.h"
#include "buzz.h"

/* ---- state machine ---- */

typedef enum {
    OA_FORWARD,
    OA_SLOW,
    OA_STOP,
    OA_BACKUP,
    OA_TURN
} OA_State;

static OA_State  state       = OA_FORWARD;
static uint16_t  timer       = 0;      /* in-state timer (ms) */
static uint16_t  trigger_cnt = 0;      /* trigger interval counter */
static uint16_t  last_dist   = 0;      /* last measured distance (mm) */

static int16_t   fwd_speed   = OA_FORWARD_SPEED;
static int16_t   slow_speed  = OA_SLOW_SPEED;

/* ---- state transition helpers ---- */

static void enter_forward(void)
{
    state = OA_FORWARD;
    pwm_car_forward(fwd_speed);
    LED_Set(LED_PRESET_FORWARD);
    Buzz_Off();
}

static void enter_slow(void)
{
    state = OA_SLOW;
    pwm_car_forward(slow_speed);
    /* 左黄慢闪 + 右黄慢闪 = 双黄慢闪 */
    LED_Set(L_YELLOW_R, L_YELLOW_G, 0, BLINK_SLOW,
            R_YELLOW_R, R_YELLOW_G, 0, BLINK_SLOW);
    Buzz_Off();
}

static void enter_stop(void)
{
    state = OA_STOP;
    timer = 0;
    pwm_car_stop();
    LED_Set(LED_PRESET_STOP);
    Buzz_On();
}

static void enter_backup(void)
{
    state = OA_BACKUP;
    timer = 0;
    pwm_car_backward(OA_BACKUP_SPEED);
    LED_Set(LED_PRESET_BACKWARD);
    Buzz_On();
}

static void enter_turn(void)
{
    state = OA_TURN;
    timer = 0;
#if OA_TURN_DIR > 0
    pwm_car_rotate_right(OA_TURN_SPEED);
    /* 右黄慢闪, 左绿稳态 */
    LED_Set(0, L_GREEN, 0, BLINK_OFF,
            R_YELLOW_R, R_YELLOW_G, 0, BLINK_SLOW);
#else
    pwm_car_rotate_left(OA_TURN_SPEED);
    /* 左黄慢闪, 右绿稳态 */
    LED_Set(L_YELLOW_R, L_YELLOW_G, 0, BLINK_SLOW,
            0, R_GREEN, 0, BLINK_OFF);
#endif
    Buzz_Off();
}

/* ---- public API ---- */

void ObstacleAvoid_Init(void)
{
    state       = OA_FORWARD;
    timer       = 0;
    trigger_cnt = 0;
    last_dist   = 0;
    fwd_speed   = OA_FORWARD_SPEED;
    slow_speed  = OA_SLOW_SPEED;
}

void ObstacleAvoid_Tick(void)
{
    /* ---- Periodic ultrasonic trigger ---- */
    if (++trigger_cnt >= OA_TRIGGER_MS) {
        trigger_cnt = 0;
        Ultrasonic_Trigger();
    }

    /* ---- Process new data when available ---- */
    if (Ultrasonic_IsReady()) {
        last_dist = Ultrasonic_GetDistance();  /* mm */

        switch (state) {
        case OA_FORWARD:
            if (last_dist > 0 && last_dist <= OA_STOP_DIST_MM) {
                enter_stop();
            } else if (last_dist > 0 && last_dist <= OA_WARN_DIST_MM) {
                enter_slow();
            } else {
                /* Open path (0 or > WARN) — keep going */
                pwm_car_forward(fwd_speed);
            }
            break;

        case OA_SLOW:
            if (last_dist > 0 && last_dist <= OA_STOP_DIST_MM) {
                enter_stop();
            } else if (last_dist >= OA_SAFE_DIST_MM) {
                enter_forward();
            } else {
                /* Still in slow zone */
                pwm_car_forward(slow_speed);
            }
            break;

        case OA_STOP:
        case OA_BACKUP:
        case OA_TURN:
            /* Timed states: data-driven transitions not needed,
               timer-based transitions handled below */
            break;
        }
    }

    /* ---- Timer-driven state transitions (every 1 ms) ---- */
    switch (state) {
    case OA_STOP:
        if (++timer >= OA_STOP_DELAY_MS) {
            enter_backup();
        }
        break;

    case OA_BACKUP:
        if (++timer >= OA_BACKUP_MS) {
            enter_turn();
        }
        break;

    case OA_TURN:
        if (++timer >= OA_TURN_MS) {
            enter_forward();
        }
        break;

    default:
        break;
    }
}

void ObstacleAvoid_SetSpeed(int16_t forward, int16_t slow)
{
    fwd_speed  = forward;
    slow_speed = slow;
}

const char *ObstacleAvoid_GetStateName(void)
{
    static const char *const names[] = {
        "FWD", "SLOW", "STOP", "BACK", "TURN"
    };
    return names[state];
}

uint16_t ObstacleAvoid_GetDistance(void)
{
    return last_dist;
}
