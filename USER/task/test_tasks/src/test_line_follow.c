/*
 * test_line_follow.c — Line tracking test with OLED display
 *
 * 加权误差 + Kp 比例差速 + EMA 滤波循迹
 *
 * 传感器电平约定：0 = 检测到黑线，1 = 白色地面
 *
 * OLED 128×32, Font_7x10 layout:
 *   Line 0: "2m4:1 0 1"     (X2 mid X4, 0=黑线 1=白地)
 *   Line 1: "F:+2  L:2500"  (EMA 滤波误差 + 左轮速度)
 */

#include "test_line_follow.h"
#include "irtracking.h"
#include "line_follow.h"
#include "line_follow_config.h"
#include "oled.h"

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

/* Helper: write uint16_t as decimal string at cursor */
static void put_u16(uint16_t n, const FontDef_t *font, OLED_Color_t color)
{
    if (n == 0) { OLED_Putc('0', font, color); return; }
    char buf[6];
    int pos = 0;
    while (n > 0) { buf[pos++] = '0' + (n % 10); n /= 10; }
    while (pos > 0) { OLED_Putc(buf[--pos], font, color); }
}

/* ---- clamping helper (matches line_follow.c) ---- */
static inline int16_t clamp(int16_t val, int16_t lo, int16_t hi)
{
    if (val < lo) return lo;
    if (val > hi) return hi;
    return val;
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
    OLED_Puts("F:-- L:----", &Font_7x10, OLED_COLOR_WHITE);
    OLED_Update();
}

void TestLineFollow_Tick(void)
{
    /* ---- 循迹运动控制（每 1 ms 运行） ---- */
    LineFollow_Run(LINE_FOLLOW_BASE_SPEED);

    /* ---- OLED 显示 ---- */
    if (++tick_cnt < LINE_FOLLOW_DISPLAY_MS) return;
    tick_cnt = 0;

    uint8_t x1 = IRTracking_Read(0);
    uint8_t x2 = IRTracking_Read(1);
    uint8_t x3 = IRTracking_Read(2);
    uint8_t x4 = IRTracking_Read(3);
    int16_t filt_err = LineFollow_GetFilteredError();

    /* 计算当前左轮速度（与 line_follow.c 一致） */
    int16_t adj = (int16_t)LINE_FOLLOW_KP * filt_err;
    int16_t left_speed;
    if (adj >= 0) {
        left_speed = clamp(LINE_FOLLOW_BASE_SPEED + adj, LINE_FOLLOW_MIN_SPEED, LINE_FOLLOW_MAX_SPEED);
    } else {
        left_speed = clamp(LINE_FOLLOW_BASE_SPEED + adj, 0, LINE_FOLLOW_MAX_SPEED);
    }

    OLED_Clear();

    /* 合并中间传感器 */
    uint8_t mid = (x1 || x3) ? 1 : 0;

    /* Line 0: "2m4:1 0 1" — X2 mid X4 (3 等效传感器) */
    OLED_GotoXY(0, 0);
    OLED_Puts("2m4:", &Font_7x10, OLED_COLOR_WHITE);
    OLED_Putc('0' + x2, &Font_7x10, OLED_COLOR_WHITE);
    OLED_Putc(' ', &Font_7x10, OLED_COLOR_WHITE);
    OLED_Putc('0' + mid, &Font_7x10, OLED_COLOR_WHITE);
    OLED_Putc(' ', &Font_7x10, OLED_COLOR_WHITE);
    OLED_Putc('0' + x4, &Font_7x10, OLED_COLOR_WHITE);

    /* Line 1: "F:+2  L:2500" — EMA 滤波误差 + 左轮速度 */
    OLED_GotoXY(0, 10);
    OLED_Puts("F:", &Font_7x10, OLED_COLOR_WHITE);
    put_int16(filt_err, &Font_7x10, OLED_COLOR_WHITE);
    OLED_Puts("  L:", &Font_7x10, OLED_COLOR_WHITE);
    put_u16((uint16_t)left_speed, &Font_7x10, OLED_COLOR_WHITE);

    OLED_Update();
}
