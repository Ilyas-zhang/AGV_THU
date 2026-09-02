/*
 * test_ir_avoid.c — IR obstacle avoidance test task with OLED
 *
 * Avoidance state machine:
 *
 *   FORWARD ──(左障碍)──► TURN_RIGHT
 *          ──(右障碍)──► TURN_LEFT
 *          ──(双障碍)──► BACKUP ──800ms──► TURN ──600ms──► FORWARD
 *
 *   TURN_LEFT  ──(无障碍)──► FORWARD
 *   TURN_RIGHT ──(无障碍)──► FORWARD
 *
 * OLED 128×32, Font_7x10 layout:
 *   Line 0: "L:0 R:0  S:FWD"    (sensors + state)
 *   Line 1: "IR Obstacle Avoid"
 */

#include "test_ir_avoid.h"
#include "ir_avoid.h"
#include "motor.h"
#include "motor_config.h"
#include "led.h"
#include "led_config.h"
#include "buzz.h"
#include "oled.h"

/* ---- configurable parameters ---- */

#define IRA_FWD_SPEED       1800    /* forward speed */
#define IRA_TURN_SPEED      1200    /* turn speed */
#define IRA_BACKUP_SPEED    900     /* backup speed */
#define IRA_BACKUP_MS       800     /* backup duration ms */
#define IRA_TURN_MS         600     /* turn duration after backup ms */
#define IRA_OLED_REFRESH    100     /* OLED refresh interval ms */

/* ---- state machine ---- */

typedef enum {
    IRA_FORWARD,
    IRA_TURN_LEFT,
    IRA_TURN_RIGHT,
    IRA_BACKUP,
    IRA_TURN
} IRA_State;

static IRA_State state = IRA_FORWARD;
static uint16_t  timer = 0;         /* in-state timer (ms) */
static uint8_t   last_sensors = 0;  /* cached sensor reading */
static uint16_t  disp_cnt = 0;      /* OLED refresh counter */

/* ---- state name lookup ---- */

static const char *state_name(IRA_State s)
{
    static const char *const names[] = {
        "FWD", "TL", "TR", "BACK", "TURN"
    };
    return names[s];
}

/* ---- state transition helpers ---- */

static void enter_forward(void)
{
    state = IRA_FORWARD;
    pwm_car_forward(IRA_FWD_SPEED);
    LED_Set(LED_PRESET_FORWARD);
    Buzz_Off();
}

static void enter_turn_left(void)
{
    state = IRA_TURN_LEFT;
    pwm_car_turn_left(IRA_TURN_SPEED);
    LED_Set(LED_PRESET_TURN_LEFT);
    Buzz_Off();
}

static void enter_turn_right(void)
{
    state = IRA_TURN_RIGHT;
    pwm_car_turn_right(IRA_TURN_SPEED);
    LED_Set(LED_PRESET_TURN_RIGHT);
    Buzz_Off();
}

static void enter_backup(void)
{
    state = IRA_BACKUP;
    timer = 0;
    pwm_car_backward(IRA_BACKUP_SPEED);
    LED_Set(LED_PRESET_STOP);       /* 双红爆闪 */
    Buzz_On();
}

static void enter_turn(void)
{
    state = IRA_TURN;
    timer = 0;
    /* After backup, default turn right; could alternate */
    pwm_car_rotate_right(IRA_TURN_SPEED);
    LED_Set(LED_PRESET_ROTATE);     /* 双黄慢闪 */
    Buzz_Off();
}

/* ---- public API ---- */

void TestIRAvoid_Init(void)
{
    IRAvoid_Init();

    state = IRA_FORWARD;
    timer = 0;
    last_sensors = 0;
    disp_cnt = 0;

    /* Initial OLED display */
    OLED_Clear();
    OLED_GotoXY(0, 0);
    OLED_Puts("L:0 R:0  S:FWD", &Font_7x10, OLED_COLOR_WHITE);
    OLED_GotoXY(0, 10);
    OLED_Puts("IR Obstacle Avoid", &Font_7x10, OLED_COLOR_WHITE);
    OLED_Update();
}

void TestIRAvoid_Tick(void)
{
    /* ---- Read sensors ---- */
    last_sensors = IRAvoid_ReadAll();    /* bit0=left, bit1=right */
    uint8_t left  = last_sensors & 0x01;
    uint8_t right = (last_sensors >> 1) & 0x01;

    /* ---- State machine ---- */
    switch (state) {
    case IRA_FORWARD:
        if (left && right) {
            /* Both sides blocked → backup */
            enter_backup();
        } else if (left) {
            /* Left obstacle → turn right to avoid */
            enter_turn_right();
        } else if (right) {
            /* Right obstacle → turn left to avoid */
            enter_turn_left();
        } else {
            /* Clear path */
            pwm_car_forward(IRA_FWD_SPEED);
        }
        break;

    case IRA_TURN_LEFT:
        if (!left && !right) {
            /* Clear → resume forward */
            enter_forward();
        }
        /* Else: keep turning left */
        break;

    case IRA_TURN_RIGHT:
        if (!left && !right) {
            /* Clear → resume forward */
            enter_forward();
        }
        /* Else: keep turning right */
        break;

    case IRA_BACKUP:
        if (++timer >= IRA_BACKUP_MS) {
            enter_turn();
        }
        break;

    case IRA_TURN:
        if (++timer >= IRA_TURN_MS) {
            enter_forward();
        }
        break;
    }

    /* ---- OLED refresh ---- */
    if (++disp_cnt >= IRA_OLED_REFRESH) {
        disp_cnt = 0;

        OLED_Clear();

        /* Line 0: "L:x R:x  S:xxx" */
        OLED_GotoXY(0, 0);
        OLED_Puts("L:", &Font_7x10, OLED_COLOR_WHITE);
        OLED_Putc('0' + left, &Font_7x10, OLED_COLOR_WHITE);
        OLED_Puts(" R:", &Font_7x10, OLED_COLOR_WHITE);
        OLED_Putc('0' + right, &Font_7x10, OLED_COLOR_WHITE);
        OLED_Puts("  S:", &Font_7x10, OLED_COLOR_WHITE);
        OLED_Puts(state_name(state), &Font_7x10, OLED_COLOR_WHITE);

        /* Line 1: "IR Obstacle Avoid" */
        OLED_GotoXY(0, 10);
        OLED_Puts("IR Obstacle Avoid", &Font_7x10, OLED_COLOR_WHITE);

        OLED_Update();
    }
}
