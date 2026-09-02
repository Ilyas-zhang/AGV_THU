/*
 * test_ir_remote.c — IR remote control car
 *
 * NEC key mapping:
 *   0x00  急停          0x80  前进          0x90  后退
 *   0x20  左转          0x60  右转          0xA0  蜂鸣器
 *   0x40  灯光          0x10  左旋90°       0x50  右旋90°
 *   0x30  加速          0x70  减速
 *
 * Speed range: IR_SPEED_MIN ~ IR_SPEED_MAX, step IR_SPEED_STEP.
 * Direction keys (前/后/左/右) respond to repeat code (长按持续运动).
 * Rotate 90° keys are timed: run motor for IR_ROTATE_MS then stop.
 *
 * OLED 128×32, Font_7x10 layout:
 *   Line 0: "K:0xXX S:yyyy R:x"   (key, speed, repeat)
 *   Line 1: "FWD" / "BCK" / ...   (current action)
 */

#include "test_ir_remote.h"
#include "ir_remote.h"
#include "motor.h"
#include "main.h"
#include "led.h"
#include "led_config.h"
#include "buzz.h"
#include "oled.h"

/* ---- Speed config ---- */
#define IR_SPEED_MIN      600
#define IR_SPEED_MAX      3600
#define IR_SPEED_STEP     10
#define IR_SPEED_DEFAULT  2100

/* ---- Rotate 90° timing ---- */
#define IR_ROTATE_MS      500    /* 估计原地旋转 90° 所需时间 */

/* ---- Key codes ---- */
#define KEY_STOP          0x00
#define KEY_FWD           0x80
#define KEY_BWD           0x90
#define KEY_LEFT          0x20
#define KEY_RIGHT         0x60
#define KEY_BUZZ          0xA0
#define KEY_LIGHT         0x40
#define KEY_ROT_L         0x10
#define KEY_ROT_R         0x50
#define KEY_SPEED_UP      0x30
#define KEY_SPEED_DOWN    0x70

/* ---- State ---- */
static int16_t  speed        = IR_SPEED_DEFAULT;
static uint8_t  buzz_on      = 0;
static uint8_t  light_on     = 0;

/* Current motion state (for speed-change re-apply) */
typedef enum {
    MOTION_STOP, MOTION_FWD, MOTION_BWD,
    MOTION_TURN_L, MOTION_TURN_R,
    MOTION_ROT_L, MOTION_ROT_R
} MotionState;
static MotionState motion = MOTION_STOP;

/* Timed rotation state */
static uint8_t  rotating     = 0;    /* 1 = in timed rotation */
static uint16_t rotate_timer = 0;

/* Turn hold state: turn auto-stops when key released (no repeat) */
#define TURN_HOLD_TIMEOUT_MS  150   /* 150ms 无 repeat → 停止转向 */
static uint16_t turn_hold_cnt = 0;

/* Current action label (for OLED) */
static const char *action_label = "---";

/* OLED refresh */
#define IR_OLED_REFRESH    100
static uint16_t disp_cnt = 0;

/* ---- Helper ---- */
static void put_hex8(uint8_t val, const FontDef_t *font, OLED_Color_t color)
{
    static const char hex[] = "0123456789ABCDEF";
    OLED_Putc(hex[val >> 4], font, color);
    OLED_Putc(hex[val & 0x0F], font, color);
}

static void put_int16(int16_t val, const FontDef_t *font, OLED_Color_t color)
{
    char buf[6];
    int pos = 0;
    if (val < 0) { OLED_Putc('-', font, color); val = -val; }
    if (val == 0) { OLED_Putc('0', font, color); return; }
    while (val > 0) { buf[pos++] = '0' + (val % 10); val /= 10; }
    while (pos > 0) OLED_Putc(buf[--pos], font, color);
}

/* ---- Action dispatch ---- */

static void do_stop(void)
{
    pwm_car_stop();
    rotating = 0;
    motion = MOTION_STOP;
    action_label = "STOP";
}

static void do_forward(void)
{
    pwm_car_forward(speed);
    rotating = 0;
    motion = MOTION_FWD;
    action_label = "FWD";
}

static void do_backward(void)
{
    pwm_car_backward(speed);
    rotating = 0;
    motion = MOTION_BWD;
    action_label = "BCK";
}

static void do_turn_left(void)
{
    pwm_car_turn_left(speed);
    rotating = 0;
    motion = MOTION_TURN_L;
    turn_hold_cnt = 0;
    action_label = "L-T";
}

static void do_turn_right(void)
{
    pwm_car_turn_right(speed);
    rotating = 0;
    motion = MOTION_TURN_R;
    turn_hold_cnt = 0;
    action_label = "R-T";
}

static void do_rotate_left(void)
{
    pwm_car_rotate_left(speed);
    rotating     = 1;
    rotate_timer = 0;
    motion = MOTION_ROT_L;
    action_label = "L-90";
}

static void do_rotate_right(void)
{
    pwm_car_rotate_right(speed);
    rotating     = 1;
    rotate_timer = 0;
    motion = MOTION_ROT_R;
    action_label = "R-90";
}

/* Re-apply current motion with updated speed */
static void reapply_motion(void)
{
    switch (motion) {
    case MOTION_FWD:     pwm_car_forward(speed);      break;
    case MOTION_BWD:     pwm_car_backward(speed);     break;
    case MOTION_TURN_L:  pwm_car_turn_left(speed);    break;
    case MOTION_TURN_R:  pwm_car_turn_right(speed);   break;
    case MOTION_ROT_L:   pwm_car_rotate_left(speed);  break;
    case MOTION_ROT_R:   pwm_car_rotate_right(speed); break;
    default:             break;  /* STOP — nothing to do */
    }
}

static void do_speed_up(void)
{
    if (speed <= IR_SPEED_MAX - IR_SPEED_STEP) {
        speed += IR_SPEED_STEP;
        reapply_motion();
    }
    action_label = "SP+";
}

static void do_speed_down(void)
{
    if (speed >= IR_SPEED_MIN + IR_SPEED_STEP) {
        speed -= IR_SPEED_STEP;
        reapply_motion();
    }
    action_label = "SP-";
}

static void do_buzz_toggle(void)
{
    buzz_on = !buzz_on;
    if (buzz_on) Buzz_On(); else Buzz_Off();
    action_label = buzz_on ? "BZ+" : "BZ-";
}

static void do_light_toggle(void)
{
    light_on = !light_on;
    if (light_on) {
        LED_Set(100, 100, 100, BLINK_OFF,
               100, 100, 100, BLINK_OFF);
    } else {
        LED_Set(0, 0, 0, BLINK_OFF,
               0, 0, 0, BLINK_OFF);
    }
    action_label = light_on ? "LT+" : "LT-";
}

/* ---- Key handler ---- */

static void handle_key(uint8_t code, uint8_t is_repeat)
{
    switch (code) {
    case KEY_STOP:
        do_stop();
        break;

    case KEY_FWD:
        /* 前进：新帧或 repeat 都执行（长按持续前进） */
        do_forward();
        break;

    case KEY_BWD:
        do_backward();
        break;

    case KEY_LEFT:
        do_turn_left();
        if (is_repeat) turn_hold_cnt = 0;  /* 长按持续转向 */
        break;

    case KEY_RIGHT:
        do_turn_right();
        if (is_repeat) turn_hold_cnt = 0;
        break;

    case KEY_ROT_L:
        if (!is_repeat) do_rotate_left();   /* 旋90°不响应repeat */
        break;

    case KEY_ROT_R:
        if (!is_repeat) do_rotate_right();
        break;

    case KEY_SPEED_UP:
        do_speed_up();
        break;

    case KEY_SPEED_DOWN:
        do_speed_down();
        break;

    case KEY_BUZZ:
        if (!is_repeat) do_buzz_toggle();
        break;

    case KEY_LIGHT:
        if (!is_repeat) do_light_toggle();
        break;

    default:
        action_label = "???";
        break;
    }
}

/* ---- Public API ---- */

void TestIRRemote_Init(void)
{
    IRRemote_Init();

    speed        = IR_SPEED_DEFAULT;
    buzz_on      = 0;
    light_on     = 0;
    motion       = MOTION_STOP;
    rotating     = 0;
    rotate_timer = 0;
    turn_hold_cnt = 0;
    action_label = "---";
    disp_cnt     = 0;

    OLED_Clear();
    OLED_GotoXY(0, 0);
    OLED_Puts("K:-- S:---- R:-", &Font_7x10, OLED_COLOR_WHITE);
    OLED_GotoXY(0, 10);
    OLED_Puts("---", &Font_7x10, OLED_COLOR_WHITE);
    OLED_Update();
}

void TestIRRemote_Tick(void)
{
    /* Drive IR frame timeout */
    IRRemote_Tick();

    /* Timed rotation: count and auto-stop */
    if (rotating) {
        if (++rotate_timer >= IR_ROTATE_MS) {
            do_stop();
        }
    }

    /* Turn hold timeout: if turning and no repeat for 150ms, auto-stop */
    if (motion == MOTION_TURN_L || motion == MOTION_TURN_R) {
        if (++turn_hold_cnt >= TURN_HOLD_TIMEOUT_MS) {
            do_stop();
        }
    }

    /* Process new IR key */
    if (IRRemote_HasNewCode()) {
        uint8_t code = IRRemote_GetCode();
        uint8_t rpt  = IRRemote_IsRepeat();
        handle_key(code, rpt);
    }

    /* OLED refresh at lower rate */
    if (++disp_cnt < IR_OLED_REFRESH) return;
    disp_cnt = 0;

    uint8_t code = IRRemote_GetCode();
    uint8_t rpt  = IRRemote_IsRepeat();

    OLED_Clear();

    /* Line 0: "K:0xXX S:yyyy R:x" */
    OLED_GotoXY(0, 0);
    OLED_Puts("K:", &Font_7x10, OLED_COLOR_WHITE);
    if (code != IRREMOTE_NO_CODE) {
        OLED_Puts("0x", &Font_7x10, OLED_COLOR_WHITE);
        put_hex8(code, &Font_7x10, OLED_COLOR_WHITE);
    } else {
        OLED_Puts("--", &Font_7x10, OLED_COLOR_WHITE);
    }
    OLED_Puts(" S:", &Font_7x10, OLED_COLOR_WHITE);
    put_int16(speed, &Font_7x10, OLED_COLOR_WHITE);
    OLED_Puts(" R:", &Font_7x10, OLED_COLOR_WHITE);
    OLED_Putc('0' + rpt, &Font_7x10, OLED_COLOR_WHITE);

    /* Line 1: action label */
    OLED_GotoXY(0, 10);
    OLED_Puts(action_label, &Font_7x10, OLED_COLOR_WHITE);

    OLED_Update();
}
