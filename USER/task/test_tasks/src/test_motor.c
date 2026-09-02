#include "test_motor.h"
#include "motor.h"
#include "key.h"
#include "led.h"
#include "led_config.h"
#include "buzz.h"

/* ---- motor test states ---- */
enum {
    STATE_FORWARD   = 0,
    STATE_BACKWARD,
    STATE_TURN_LEFT,
    STATE_TURN_RIGHT,
    STATE_ROTATE,
    STATE_STOP,
    STATE_COUNT     /* 6 */
};

static uint8_t motor_state = STATE_STOP;

/* ---- set LED according to motor state ---- */
static void update_led(void)
{
    switch (motor_state) {
    case STATE_FORWARD:    LED_Set(LED_PRESET_FORWARD);    break;
    case STATE_BACKWARD:   LED_Set(LED_PRESET_BACKWARD);   break;
    case STATE_TURN_LEFT:  LED_Set(LED_PRESET_TURN_LEFT);  break;
    case STATE_TURN_RIGHT: LED_Set(LED_PRESET_TURN_RIGHT); break;
    case STATE_ROTATE:     LED_Set(LED_PRESET_ROTATE);     break;
    case STATE_STOP:       LED_Set(LED_PRESET_STOP);       break;
    default:               LED_Set(LED_PRESET_STOP);       break;
    }
}

/* ---- public API ---- */

void TestMotor_Init(void)
{
    motor_state = STATE_STOP;
    car_stop();
    Buzz_Off();
    update_led();
}

void TestMotor_Tick(void)
{
    if (Key_RisingEdge(3)) {
        motor_state = (motor_state + 1) % STATE_COUNT;
        switch (motor_state) {
        case STATE_FORWARD:    car_forward();      break;
        case STATE_BACKWARD:   car_backward();     break;
        case STATE_TURN_LEFT:  car_turn_left();    break;
        case STATE_TURN_RIGHT: car_turn_right();   break;
        case STATE_ROTATE:     car_rotate_left();  break;
        case STATE_STOP:       car_stop();         break;
        default:               car_stop();         break;
        }
        /* 倒车蜂鸣器响，其余关闭 */
        if (motor_state == STATE_BACKWARD) Buzz_On();
        else                               Buzz_Off();
        update_led();
    }
}
