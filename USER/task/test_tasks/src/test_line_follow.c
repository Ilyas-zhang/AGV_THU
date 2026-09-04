/*
 * test_line_follow.c — Line tracking test with OLED display + motor control
 *
 * 每 100 ms 刷新 OLED 显示 4 路循迹传感器状态和加权误差，
 * 同时驱动循迹运动逻辑（比例差速转向）。
 *
 * 传感器电平约定：0 = 检测到黑线，1 = 白色地面
 *
 * OLED 128×32, Font_7x10 layout:
 *   Line 0: "X: 1 1 0 1"     (X2 X1 X3 X4 物理左→右, 0=黑线 1=白地)
 *   Line 1: "ST:06 E:+1"     (十六进制状态码 + 加权误差)
 */

#include "test_line_follow.h"
#include "irtracking.h"
#include "line_follow.h"
#include "line_follow_config.h"
#include "oled.h"

static uint16_t tick_cnt = 0;

/* Helper: write int8_t as signed decimal string at cursor */
static void put_int8(int8_t n, const FontDef_t *font, OLED_Color_t color)
{
    if (n < 0) {
        OLED_Putc('-', font, color);
        n = -n;
    }
    if (n == 0) { OLED_Putc('0', font, color); return; }
    char buf[4];
    int pos = 0;
    while (n > 0) { buf[pos++] = '0' + (n % 10); n /= 10; }
    while (pos > 0) { OLED_Putc(buf[--pos], font, color); }
}

/* Helper: 写单个十六进制字符 */
static void put_hex4(uint8_t n, const FontDef_t *font, OLED_Color_t color)
{
    OLED_Putc((n < 10) ? ('0' + n) : ('A' + n - 10), font, color);
}

void TestLineFollow_Init(void)
{
    LineFollow_Init();
    OLED_Init();

    tick_cnt = 0;

    OLED_Clear();
    OLED_GotoXY(0, 0);
    OLED_Puts("X: - - - -", &Font_7x10, OLED_COLOR_WHITE);
    OLED_GotoXY(0, 10);
    OLED_Puts("ST:-- E:--", &Font_7x10, OLED_COLOR_WHITE);
    OLED_Update();
}

void TestLineFollow_Tick(void)
{
    /* ---- 循迹运动控制（每 1 ms 运行） ---- */
    LineFollow_Run(LINE_FOLLOW_BASE_SPEED);   /* 循迹运动控制 */

    /* ---- OLED 显示（每 100 ms 刷新） ---- */
    if (++tick_cnt < LINE_FOLLOW_DISPLAY_MS) return;
    tick_cnt = 0;

    uint8_t state = IRTracking_ReadAll();
    uint8_t x1 = IRTracking_Read(0);
    uint8_t x2 = IRTracking_Read(1);
    uint8_t x3 = IRTracking_Read(2);
    uint8_t x4 = IRTracking_Read(3);
    int8_t error = LineFollow_GetError();

    OLED_Clear();

    /* Line 0: "X: 1 1 0 1" — 物理顺序 X2 X1 X3 X4 (左→右) */
    OLED_GotoXY(0, 0);
    OLED_Puts("X:", &Font_7x10, OLED_COLOR_WHITE);
    OLED_Putc('0' + x2, &Font_7x10, OLED_COLOR_WHITE);
    OLED_Putc(' ', &Font_7x10, OLED_COLOR_WHITE);
    OLED_Putc('0' + x1, &Font_7x10, OLED_COLOR_WHITE);
    OLED_Putc(' ', &Font_7x10, OLED_COLOR_WHITE);
    OLED_Putc('0' + x3, &Font_7x10, OLED_COLOR_WHITE);
    OLED_Putc(' ', &Font_7x10, OLED_COLOR_WHITE);
    OLED_Putc('0' + x4, &Font_7x10, OLED_COLOR_WHITE);

    /* Line 1: "ST:06 E:+1" */
    OLED_GotoXY(0, 10);
    OLED_Puts("ST:", &Font_7x10, OLED_COLOR_WHITE);
    put_hex4(state >> 4, &Font_7x10, OLED_COLOR_WHITE);
    put_hex4(state & 0x0F, &Font_7x10, OLED_COLOR_WHITE);
    OLED_Puts(" E:", &Font_7x10, OLED_COLOR_WHITE);
    put_int8(error, &Font_7x10, OLED_COLOR_WHITE);

    OLED_Update();
}
