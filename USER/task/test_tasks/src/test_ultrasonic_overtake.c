/*
 * test_ultrasonic_overtake.c — Ultrasonic overtaking test with OLED display
 *
 * Drives the overtaking state machine and displays distance + state + timer on OLED.
 *
 * OLED 128×32, Font_7x10 layout:
 *   Line 0: "D:xxxcm S:FWD   "
 *   Line 1: "T:xxxx STP:15   "  (in-state timer + stop threshold)
 */

#include "test_ultrasonic_overtake.h"
#include "ultrasonic_overtake.h"
#include "ultrasonic.h"
#include "oled.h"

#define UO_OLED_REFRESH    50      /* OLED refresh interval ms (faster for debug) */

static uint16_t disp_cnt = 0;

/* Helper: write uint16_t as decimal at cursor */
static void put_uint16(uint16_t n, const FontDef_t *font, OLED_Color_t color)
{
    if (n == 0) { OLED_Putc('0', font, color); return; }
    char buf[6];
    int pos = 0;
    while (n > 0) { buf[pos++] = '0' + (n % 10); n /= 10; }
    while (pos > 0) { OLED_Putc(buf[--pos], font, color); }
}

void TestUltrasonicOvertake_Init(void)
{
    UltrasonicOvertake_Init();

    disp_cnt = 0;

    /* Initial OLED display */
    OLED_Clear();
    OLED_GotoXY(0, 0);
    OLED_Puts("D:--cm S:---    ", &Font_7x10, OLED_COLOR_WHITE);
    OLED_GotoXY(0, 10);
    OLED_Puts("T:---- STP:15  ", &Font_7x10, OLED_COLOR_WHITE);
    OLED_Update();
}

void TestUltrasonicOvertake_Tick(void)
{
    /* Drive the overtaking state machine (includes ultrasonic trigger) */
    UltrasonicOvertake_Tick();

    if (++disp_cnt < UO_OLED_REFRESH) return;
    disp_cnt = 0;

    uint16_t dist_cm = (UltrasonicOvertake_GetDistance() + 5) / 10;
    const char *state_name = UltrasonicOvertake_GetStateName();
    uint16_t timer_val = UltrasonicOvertake_GetTimer();

    OLED_Clear();

    /* Line 0: "D:xxxcm S:FWD   " */
    OLED_GotoXY(0, 0);
    OLED_Puts("D:", &Font_7x10, OLED_COLOR_WHITE);
    put_uint16(dist_cm, &Font_7x10, OLED_COLOR_WHITE);
    OLED_Puts("cm S:", &Font_7x10, OLED_COLOR_WHITE);
    OLED_Puts(state_name, &Font_7x10, OLED_COLOR_WHITE);

    /* Line 1: "T:xxxx STP:15   " (timer debug) */
    OLED_GotoXY(0, 10);
    OLED_Puts("T:", &Font_7x10, OLED_COLOR_WHITE);
    put_uint16(timer_val, &Font_7x10, OLED_COLOR_WHITE);
    OLED_Puts(" STP:15", &Font_7x10, OLED_COLOR_WHITE);

    OLED_Update();
}
