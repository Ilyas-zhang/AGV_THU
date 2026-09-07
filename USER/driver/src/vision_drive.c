/*
 * vision_drive.c — Vision-driven overtaking driver
 *
 * Receives road sign commands from K210 via USART2 (k210_comm.c ISR),
 * executes overtaking maneuvers:
 *   LEFT  sign → left overtake  → forward 1s → right overtake → resume
 *   RIGHT sign → right overtake → forward 1s → left overtake  → resume
 *   STOP  sign → stop
 *
 * State machine:
 *   VD_IDLE     → waiting for sign, car forward
 *   VD_OVT_1    → first overtake in progress
 *   VD_FWD_WAIT → driving forward between overtakes (timer)
 *   VD_OVT_2    → second overtake in progress (return to lane)
 *   VD_STOPPED  → car stopped (STOP sign)
 */

#include "vision_drive.h"
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
    VD_STOPPED
} VD_State;

static VD_State  vd_state    = VD_IDLE;
static int8_t    vd_dir      = 0;          /* first overtake direction */
static uint16_t  fwd_timer   = 0;         /* forward wait counter (ms) */
static char      last_sign   = '-';       /* last received sign command */

/* ---- helpers ---- */

static const char *vd_state_names[] = {
    "IDLE", "OVT1", "FWD", "OVT2", "STOP"
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
    }

    /* ---- Process K210 sign commands ---- */
    if (K210Comm_HasMessage()) {
        const char *msg = K210Comm_GetMessage();
        K210Comm_ClearFlag();

        /* Record last sign for display */
        if (msg[0] == 'R' || msg[0] == 'L' || msg[0] == 'S'
            || msg[0] == 'a') {
            last_sign = msg[0];
        }

        /* Only accept new maneuver when idle or stopped */
        if (vd_state == VD_IDLE || vd_state == VD_STOPPED) {
            if (msg[0] == 'L') {
                start_maneuver(OVERTAKE_DIR_LEFT);
            } else if (msg[0] == 'R') {
                start_maneuver(OVERTAKE_DIR_RIGHT);
            } else if (msg[0] == 'S') {
                pwm_car_stop();
                LED_Set(LED_PRESET_STOP);
                Buzz_On();
                vd_state = VD_STOPPED;
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
