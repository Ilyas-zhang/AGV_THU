/*
 * test_ultrasonic.c — Ultrasonic ranging + speed test with OLED display
 *
 * Every 60 ms triggers HC-SR04 measurement.
 * Displays distance and speed on OLED.
 *
 * OLED 128×32, Font_7x10: ~3 lines × ~18 chars
 *   Line 0: "D: xxx cm"    (distance)
 *   Line 1: "V: xxx cm/s"  (speed, EMA filtered)
 */

#include "test_ultrasonic.h"
#include "ultrasonic.h"
#include "ultrasonic_speed.h"
#include "oled.h"

#define TRIGGER_INTERVAL_MS  60

static uint16_t tick_cnt = 0;

/* Helper: write int16_t as signed decimal string at cursor */
static void put_int16(int16_t n, const FontDef_t *font, OLED_Color_t color)
{
    if (n < 0) {
        OLED_Putc('-', font, color);
        n = -n;
    }
    if (n == 0) { OLED_Putc('0', font, color); return; }
    char buf[6];
    int pos = 0;
    while (n > 0) { buf[pos++] = '0' + (n % 10); n /= 10; }
    while (pos > 0) { OLED_Putc(buf[--pos], font, color); }
}

void TestUltrasonic_Init(void)
{
    UltrasonicSpeed_Init();

    OLED_Clear();
    OLED_GotoXY(0, 0);
    OLED_Puts("D: -- cm", &Font_7x10, OLED_COLOR_WHITE);
    OLED_GotoXY(0, 10);
    OLED_Puts("V: -- cm/s", &Font_7x10, OLED_COLOR_WHITE);
    OLED_Update();

    tick_cnt = 0;
}

void TestUltrasonic_Tick(void)
{
    /* ---- Periodic trigger ---- */
    if (++tick_cnt >= TRIGGER_INTERVAL_MS) {
        tick_cnt = 0;
        Ultrasonic_Trigger();
    }

    /* ---- Display when new data arrives ---- */
    if (Ultrasonic_IsReady()) {
        uint16_t dist_mm = Ultrasonic_GetDistance();
        uint16_t dist_cm = Ultrasonic_GetDistanceCm();

        /* Feed speed estimator */
        UltrasonicSpeed_Update(dist_mm);
        int16_t speed_cm_s = UltrasonicSpeed_GetCmPerS();

        /* Clear and redraw both lines */
        OLED_Clear();

        OLED_GotoXY(0, 0);
        OLED_Puts("D: ", &Font_7x10, OLED_COLOR_WHITE);
        put_int16((int16_t)dist_cm, &Font_7x10, OLED_COLOR_WHITE);
        OLED_Puts(" cm", &Font_7x10, OLED_COLOR_WHITE);

        OLED_GotoXY(0, 10);
        OLED_Puts("V: ", &Font_7x10, OLED_COLOR_WHITE);
        put_int16(speed_cm_s, &Font_7x10, OLED_COLOR_WHITE);
        OLED_Puts(" cm/s", &Font_7x10, OLED_COLOR_WHITE);

        OLED_Update();
    }
}
