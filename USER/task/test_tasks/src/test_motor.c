#include "test_motor.h"
#include "motor.h"
#include "motor_config.h"
#include "key.h"
#include "led.h"
#include "led_config.h"
#include "buzz.h"
#include "oled.h"

/* ---- motor test states ---- */
enum {
    STATE_FORWARD   = 0,
    STATE_BACKWARD,
    STATE_TURN_LEFT,
    STATE_TURN_RIGHT,
    STATE_ROTATE,
    STATE_STOP,
    STATE_PWM_FWD,      /* PWM 调速前进 */
    STATE_PWM_BWD,      /* PWM 调速后退 */
    STATE_COUNT         /* 8 */
};

/* PWM 测试速度表（按 KEY2 切换） */
static const int16_t pwm_speeds[] = {
    600, 900, 1200, 1500, 1800, 2100, 2400, 2700, 3000, 3599
};
#define PWM_SPEED_COUNT  ((int)(sizeof(pwm_speeds) / sizeof(pwm_speeds[0])))

static uint8_t  motor_state = STATE_STOP;
static uint8_t  speed_idx   = 4;       /* 默认 pwm_speeds[4] = 1800 */

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
    case STATE_PWM_FWD:    LED_Set(LED_PRESET_FORWARD);     break;
    case STATE_PWM_BWD:    LED_Set(LED_PRESET_BACKWARD);    break;
    default:               LED_Set(LED_PRESET_STOP);       break;
    }
}

/* ---- OLED helpers ---- */
#define OLED_REFRESH    200
static uint16_t disp_cnt = 0;

static void put_int16(int16_t val, const FontDef_t *font, OLED_Color_t color)
{
    char buf[6];
    int pos = 0;
    if (val < 0) { OLED_Putc('-', font, color); val = -val; }
    if (val == 0) { OLED_Putc('0', font, color); return; }
    while (val > 0) { buf[pos++] = '0' + (val % 10); val /= 10; }
    while (pos > 0) OLED_Putc(buf[--pos], font, color);
}

static const char *state_name(void)
{
    switch (motor_state) {
    case STATE_FORWARD:    return "FWD";
    case STATE_BACKWARD:   return "BCK";
    case STATE_TURN_LEFT:  return "L-T";
    case STATE_TURN_RIGHT: return "R-T";
    case STATE_ROTATE:     return "ROT";
    case STATE_STOP:       return "STP";
    case STATE_PWM_FWD:    return "PF ";
    case STATE_PWM_BWD:    return "PB ";
    default:               return "???";
    }
}

/* ---- public API ---- */

void TestMotor_Init(void)
{
    motor_state = STATE_STOP;
    speed_idx   = 4;
    car_stop();
    Buzz_Off();
    update_led();

    OLED_Clear();
    OLED_GotoXY(0, 0);
    OLED_Puts("Motor Test", &Font_7x10, OLED_COLOR_WHITE);
    OLED_Update();
}

void TestMotor_Tick(void)
{
    /* KEY3: 切换运动状态 */
    if (Key_RisingEdge(3)) {
        motor_state = (motor_state + 1) % STATE_COUNT;
        int16_t spd = pwm_speeds[speed_idx];
        switch (motor_state) {
        case STATE_FORWARD:    car_forward();            break;
        case STATE_BACKWARD:   car_backward();           break;
        case STATE_TURN_LEFT:  car_turn_left();          break;
        case STATE_TURN_RIGHT: car_turn_right();         break;
        case STATE_ROTATE:     car_rotate_left();        break;
        case STATE_STOP:       car_stop();               break;
        case STATE_PWM_FWD:    pwm_car_forward(spd);     break;
        case STATE_PWM_BWD:    pwm_car_backward(spd);    break;
        default:               car_stop();               break;
        }
        if (motor_state == STATE_BACKWARD || motor_state == STATE_PWM_BWD)
            Buzz_On();
        else
            Buzz_Off();
        update_led();
    }

    /* KEY2: 切换 PWM 速度（仅在 PWM 模式下生效） */
    if (Key_RisingEdge(2)) {
        speed_idx = (speed_idx + 1) % PWM_SPEED_COUNT;
        int16_t spd = pwm_speeds[speed_idx];
        if (motor_state == STATE_PWM_FWD) {
            pwm_car_forward(spd);
        } else if (motor_state == STATE_PWM_BWD) {
            pwm_car_backward(spd);
        }
    }

    /* KEY1: 紧急停止 */
    if (Key_RisingEdge(1)) {
        motor_state = STATE_STOP;
        car_stop();
        Buzz_Off();
        update_led();
    }

    /* OLED 刷新 */
    if (++disp_cnt < OLED_REFRESH) return;
    disp_cnt = 0;

    int16_t spd = pwm_speeds[speed_idx];

    OLED_Clear();
    OLED_GotoXY(0, 0);
    OLED_Puts((char *)state_name(), &Font_7x10, OLED_COLOR_WHITE);
    OLED_Puts(" PWM:", &Font_7x10, OLED_COLOR_WHITE);
    put_int16(spd, &Font_7x10, OLED_COLOR_WHITE);

    OLED_GotoXY(0, 10);
    OLED_Puts("K3:mode K2:spd K1:stp", &Font_7x10, OLED_COLOR_WHITE);

    OLED_Update();
}
