/*
 * test_encoder.c — Encoder test with OLED display
 *
 * Every 100 ms refreshes OLED with 4-channel encoder delta counts.
 * Useful for verifying encoder wiring and direction:
 *   - Manually turn each wheel, check delta changes
 *   - Forward direction should give positive delta
 *   - If negative, set ENCODERx_REVERSE = 1 in encoder_config.h
 *
 * OLED 128×32, Font_7x10: ~3 lines × ~18 chars
 *   Line 0: "E1:xxxxx E2:xxxxx"  (delta counts)
 *   Line 1: "E3:xxxxx E4:xxxxx"  (delta counts)
 */

#include "test_encoder.h"
#include "encoder.h"
#include "oled.h"

#define DISPLAY_INTERVAL_MS  100

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

void TestEncoder_Init(void)
{
    OLED_Clear();
    OLED_GotoXY(0, 0);
    OLED_Puts("E1:0    E2:0", &Font_7x10, OLED_COLOR_WHITE);
    OLED_GotoXY(0, 10);
    OLED_Puts("E3:0    E4:0", &Font_7x10, OLED_COLOR_WHITE);
    OLED_Update();

    tick_cnt = 0;
}

void TestEncoder_Tick(void)
{
    if (++tick_cnt < DISPLAY_INTERVAL_MS) return;
    tick_cnt = 0;

    int16_t deltas[4];
    Encoder_GetAllDeltas(deltas);

    OLED_Clear();

    /* Line 0: E1 and E2 */
    OLED_GotoXY(0, 0);
    OLED_Puts("E1:", &Font_7x10, OLED_COLOR_WHITE);
    put_int16(deltas[0], &Font_7x10, OLED_COLOR_WHITE);
    OLED_Puts(" E2:", &Font_7x10, OLED_COLOR_WHITE);
    put_int16(deltas[1], &Font_7x10, OLED_COLOR_WHITE);

    /* Line 1: E3 and E4 */
    OLED_GotoXY(0, 10);
    OLED_Puts("E3:", &Font_7x10, OLED_COLOR_WHITE);
    put_int16(deltas[2], &Font_7x10, OLED_COLOR_WHITE);
    OLED_Puts(" E4:", &Font_7x10, OLED_COLOR_WHITE);
    put_int16(deltas[3], &Font_7x10, OLED_COLOR_WHITE);

    OLED_Update();
}
