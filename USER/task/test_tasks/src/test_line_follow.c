/*
 * test_line_follow.c — 四路循迹测试任务
 *
 * 控制循环每 1 ms 运行一次。默认关闭 OLED 刷新，避免阻塞式 I2C
 * 占用 SysTick 时间；调试时可在 line_follow_config.h 打开显示。
 *
 * 传感器电平：0 = 黑线，1 = 白地
 * OLED 物理顺序：X2 X1 X3 X4（左→右）
 */

#include "test_line_follow.h"
#include "irtracking.h"
#include "line_follow.h"
#include "line_follow_config.h"

#if LINE_FOLLOW_OLED_ENABLE
#include "oled.h"
#endif

static uint16_t tick_cnt = 0;

#if LINE_FOLLOW_OLED_ENABLE
static void put_int8(int8_t n, const FontDef_t *font, OLED_Color_t color)
{
    if (n < 0) {
        OLED_Putc('-', font, color);
        n = (int8_t)-n;
    } else if (n > 0) {
        OLED_Putc('+', font, color);
    }

    if (n == 0) {
        OLED_Putc('0', font, color);
        return;
    }

    OLED_Putc((char)('0' + n), font, color);
}

static char turn_char(int8_t dir)
{
    if (dir < 0) return 'L';
    if (dir > 0) return 'R';
    return 'F';
}
#endif

void TestLineFollow_Init(void)
{
    LineFollow_Init();
    tick_cnt = 0;

#if LINE_FOLLOW_OLED_ENABLE
    OLED_Init();
    OLED_Clear();
    OLED_GotoXY(0, 0);
    OLED_Puts("X2 X1 X3 X4", &Font_7x10, OLED_COLOR_WHITE);
    OLED_GotoXY(0, 10);
    OLED_Puts("E:0 D:F", &Font_7x10, OLED_COLOR_WHITE);
    OLED_Update();
#endif
}

void TestLineFollow_Tick(void)
{
    /* 传感器已在 SysTick 中先由 IRTracking_Tick() 更新，这里直接用最新结果。 */
    LineFollow_Run(LINE_FOLLOW_BASE_SPEED);

#if LINE_FOLLOW_OLED_ENABLE
    if (++tick_cnt < LINE_FOLLOW_DISPLAY_MS) return;
    tick_cnt = 0;

    uint8_t x1 = IRTracking_Read(0);
    uint8_t x2 = IRTracking_Read(1);
    uint8_t x3 = IRTracking_Read(2);
    uint8_t x4 = IRTracking_Read(3);
    int8_t error = LineFollow_GetError();
    int8_t dir = LineFollow_GetTurnDirection();

    OLED_Clear();

    OLED_GotoXY(0, 0);
    OLED_Putc((char)('0' + x2), &Font_7x10, OLED_COLOR_WHITE);
    OLED_Putc(' ', &Font_7x10, OLED_COLOR_WHITE);
    OLED_Putc((char)('0' + x1), &Font_7x10, OLED_COLOR_WHITE);
    OLED_Putc(' ', &Font_7x10, OLED_COLOR_WHITE);
    OLED_Putc((char)('0' + x3), &Font_7x10, OLED_COLOR_WHITE);
    OLED_Putc(' ', &Font_7x10, OLED_COLOR_WHITE);
    OLED_Putc((char)('0' + x4), &Font_7x10, OLED_COLOR_WHITE);

    OLED_GotoXY(0, 10);
    OLED_Puts("E:", &Font_7x10, OLED_COLOR_WHITE);
    put_int8(error, &Font_7x10, OLED_COLOR_WHITE);
    OLED_Puts(" D:", &Font_7x10, OLED_COLOR_WHITE);
    OLED_Putc(turn_char(dir), &Font_7x10, OLED_COLOR_WHITE);

    OLED_Update();
#else
    (void)tick_cnt;
#endif
}
