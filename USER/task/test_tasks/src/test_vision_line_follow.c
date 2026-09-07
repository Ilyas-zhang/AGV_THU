/*
 * test_vision_line_follow.c — 视觉循迹测试 + OLED 显示
 *
 * K210 发线偏差 → STM32 差速控制
 *
 * OLED 128×32, Font_7x10 layout:
 *   Line 0: "E:+30  OFF:0"   (滤波误差 + 脱线标志)
 *   Line 1: "L:2500 R:3599"   (左右轮速度)
 */

#include "test_vision_line_follow.h"
#include "vision_line_follow.h"
#include "vision_line_follow_config.h"
#include "oled.h"

static uint16_t tick_cnt = 0;

/* Helper: write int16_t as signed decimal */
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

/* Helper: write uint16_t as decimal */
static void put_u16(uint16_t n, const FontDef_t *font, OLED_Color_t color)
{
    if (n == 0) { OLED_Putc('0', font, color); return; }
    char buf[6];
    int pos = 0;
    while (n > 0) { buf[pos++] = '0' + (n % 10); n /= 10; }
    while (pos > 0) { OLED_Putc(buf[--pos], font, color); }
}

/* ---- clamping (matches driver) ---- */
static inline int16_t clamp(int16_t val, int16_t lo, int16_t hi)
{
    if (val < lo) return lo;
    if (val > hi) return hi;
    return val;
}

void TestVisionLineFollow_Init(void)
{
    VisionLineFollow_Init();
    OLED_Init();

    tick_cnt = 0;

    OLED_Clear();
    OLED_GotoXY(0, 0);
    OLED_Puts("E:--- OFF:-", &Font_7x10, OLED_COLOR_WHITE);
    OLED_GotoXY(0, 10);
    OLED_Puts("L:---- R:----", &Font_7x10, OLED_COLOR_WHITE);
    OLED_Update();
}

void TestVisionLineFollow_Tick(void)
{
    /* ---- 视觉循迹控制（每 1 ms） ---- */
    VisionLineFollow_Tick();

    /* ---- OLED 显示 ---- */
    if (++tick_cnt < VLF_DISPLAY_MS) return;
    tick_cnt = 0;

    int16_t filt = VisionLineFollow_GetFilteredError();
    uint8_t off  = VisionLineFollow_IsOffline();

    /* 计算左右轮速度（与 driver 一致） */
    int16_t left_speed, right_speed;
    if (off) {
        left_speed  = 0;
        right_speed = 0;
    } else {
        int16_t adj = (int16_t)VLF_KP * filt;
        if (adj >= 0) {
            left_speed  = clamp(VLF_BASE_SPEED + adj, VLF_MIN_SPEED, VLF_MAX_SPEED);
            right_speed = clamp(VLF_BASE_SPEED - adj, 0, VLF_MAX_SPEED);
        } else {
            left_speed  = clamp(VLF_BASE_SPEED + adj, 0, VLF_MAX_SPEED);
            right_speed = clamp(VLF_BASE_SPEED - adj, VLF_MIN_SPEED, VLF_MAX_SPEED);
        }
    }

    OLED_Clear();

    /* Line 0: "E:+30  OFF:0" */
    OLED_GotoXY(0, 0);
    OLED_Puts("E:", &Font_7x10, OLED_COLOR_WHITE);
    put_int16(filt, &Font_7x10, OLED_COLOR_WHITE);
    OLED_Puts("  OFF:", &Font_7x10, OLED_COLOR_WHITE);
    OLED_Putc('0' + off, &Font_7x10, OLED_COLOR_WHITE);

    /* Line 1: "L:2500 R:3599" */
    OLED_GotoXY(0, 10);
    OLED_Puts("L:", &Font_7x10, OLED_COLOR_WHITE);
    put_u16((uint16_t)left_speed, &Font_7x10, OLED_COLOR_WHITE);
    OLED_Puts(" R:", &Font_7x10, OLED_COLOR_WHITE);
    put_u16((uint16_t)right_speed, &Font_7x10, OLED_COLOR_WHITE);

    OLED_Update();
}
