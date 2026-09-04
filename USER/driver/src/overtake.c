/*
 * overtake.c — Generic overtaking maneuver driver
 *
 * Left overtake:  STOP → ROTATE_LEFT  → PASS → ROTATE_RIGHT → IDLE
 * Right overtake: STOP → ROTATE_RIGHT → PASS → ROTATE_LEFT  → IDLE
 *
 * 物理接线反向：代码 rotate_right = 实际左旋, 代码 rotate_left = 实际右旋
 */

#include "overtake.h"
#include "overtake_config.h"
#include "motor.h"
#include "led.h"
#include "led_config.h"
#include "buzz.h"

/* ---- state machine ---- */

typedef enum {
    OV_IDLE,
    OV_STOP,
    OV_ROTATE_1,    /* first rotate (direction-dependent) */
    OV_PASS,
    OV_ROTATE_2     /* second rotate (opposite direction) */
} OV_State;

static OV_State   state      = OV_IDLE;
static uint16_t   timer      = 0;
static int8_t     direction  = 0;       /* OVERTAKE_DIR_LEFT or RIGHT, 0 = idle */
static uint8_t    complete   = 0;

/* ---- rotate helpers (with physical wiring inversion) ---- */

static void do_rotate_left(void)
{
    pwm_car_rotate_right(OV_ROTATE_SPEED);  /* 物理接线反向 */
}

static void do_rotate_right(void)
{
    pwm_car_rotate_left(OV_ROTATE_SPEED);   /* 物理接线反向 */
}

/* ---- state transition helpers ---- */

static void enter_idle(void)
{
    state = OV_IDLE;
    timer = 0;
    direction = 0;
}

static void enter_stop(void)
{
    state = OV_STOP;
    timer = 0;
    pwm_car_stop();
    LED_Set(LED_PRESET_STOP);
    Buzz_On();
}

static void enter_rotate_1(void)
{
    state = OV_ROTATE_1;
    timer = 0;
    if (direction == OVERTAKE_DIR_LEFT) {
        do_rotate_left();
    } else {
        do_rotate_right();
    }
    LED_Set(LED_PRESET_ROTATE);
    Buzz_Off();
}

static void enter_pass(void)
{
    state = OV_PASS;
    timer = 0;
    pwm_car_forward(OV_PASS_SPEED);
    LED_Set(LED_PRESET_FORWARD);
    Buzz_Off();
}

static void enter_rotate_2(void)
{
    state = OV_ROTATE_2;
    timer = 0;
    if (direction == OVERTAKE_DIR_LEFT) {
        do_rotate_right();   /* return to original heading */
    } else {
        do_rotate_left();    /* return to original heading */
    }
    LED_Set(LED_PRESET_ROTATE);
    Buzz_Off();
}

/* ---- public API ---- */

void Overtake_Init(void)
{
    state    = OV_IDLE;
    timer    = 0;
    direction = 0;
    complete = 0;
}

void Overtake_Trigger(int8_t dir)
{
    if (state == OV_IDLE) {
        direction = dir;
        enter_stop();
    }
}

void Overtake_Tick(void)
{
    complete = 0;

    switch (state) {
    case OV_IDLE:
        break;

    case OV_STOP:
        if (++timer >= OV_STOP_DELAY_MS) {
            enter_rotate_1();
        }
        break;

    case OV_ROTATE_1:
        if (++timer >= OV_ROTATE_MS) {
            enter_pass();
        }
        break;

    case OV_PASS:
        if (++timer >= OV_PASS_MS) {
            enter_rotate_2();
        }
        break;

    case OV_ROTATE_2:
        if (++timer >= OV_ROTATE_MS) {
            complete = 1;
            enter_idle();
        }
        break;
    }
}

uint8_t Overtake_IsActive(void)
{
    return (state != OV_IDLE) ? 1 : 0;
}

uint8_t Overtake_IsComplete(void)
{
    return complete;
}

const char *Overtake_GetStateName(void)
{
    static const char *const names[] = {
        "IDLE", "STOP", "ROT1", "PASS", "ROT2"
    };
    return names[state];
}

uint16_t Overtake_GetTimer(void)
{
    return timer;
}

int8_t Overtake_GetDirection(void)
{
    if (state == OV_IDLE) return 0;
    return direction;
}
