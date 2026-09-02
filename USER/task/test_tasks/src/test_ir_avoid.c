/*
 * test_ir_avoid.c — IR obstacle avoidance sensor test (display only)
 *
 * Reads left/right IR avoidance sensors and displays on OLED.
 * NO motor control — pure sensor reading + display.
 *
 * OLED 128×32, Font_7x10 layout:
 *   Line 0: "L:x R:x   IR Avoid"   (x = 1 obstacle, 0 clear)
 *   Line 1: "LF:0 RF:0"             (raw GPIO level for debug)
 */

#include "test_ir_avoid.h"
#include "ir_avoid.h"
#include "oled.h"
#include "main.h"

#define IRA_OLED_REFRESH    100     /* OLED refresh interval ms */

static uint16_t disp_cnt = 0;

void TestIRAvoid_Init(void)
{
    IRAvoid_Init();

    disp_cnt = 0;

    /* Initial OLED display */
    OLED_Clear();
    OLED_GotoXY(0, 0);
    OLED_Puts("L:- R:-  IR Avoid", &Font_7x10, OLED_COLOR_WHITE);
    OLED_GotoXY(0, 10);
    OLED_Puts("LF:- RF:-", &Font_7x10, OLED_COLOR_WHITE);
    OLED_Update();
}

void TestIRAvoid_Tick(void)
{
    if (++disp_cnt < IRA_OLED_REFRESH) return;
    disp_cnt = 0;

    /* Read sensors (1 = obstacle, 0 = clear) */
    uint8_t left  = IRAvoid_ReadLeft();
    uint8_t right = IRAvoid_ReadRight();

    /* Raw GPIO level for debug (0 = LOW, 1 = HIGH) */
    uint8_t raw_l = (HAL_GPIO_ReadPin(IRAVOID_L_GPIO_Port, IRAVOID_L_Pin) != GPIO_PIN_RESET) ? 1 : 0;
    uint8_t raw_r = (HAL_GPIO_ReadPin(IRAVOID_R_GPIO_Port, IRAVOID_R_Pin) != GPIO_PIN_RESET) ? 1 : 0;

    OLED_Clear();

    /* Line 0: "L:x R:x   IR Avoid" */
    OLED_GotoXY(0, 0);
    OLED_Puts("L:", &Font_7x10, OLED_COLOR_WHITE);
    OLED_Putc('0' + left, &Font_7x10, OLED_COLOR_WHITE);
    OLED_Puts(" R:", &Font_7x10, OLED_COLOR_WHITE);
    OLED_Putc('0' + right, &Font_7x10, OLED_COLOR_WHITE);
    OLED_Puts("  IR Avoid", &Font_7x10, OLED_COLOR_WHITE);

    /* Line 1: "LF:x RF:x" (raw level — if LF always 1, PF9 may be floating/wrong pin) */
    OLED_GotoXY(0, 10);
    OLED_Puts("LF:", &Font_7x10, OLED_COLOR_WHITE);
    OLED_Putc('0' + raw_l, &Font_7x10, OLED_COLOR_WHITE);
    OLED_Puts(" RF:", &Font_7x10, OLED_COLOR_WHITE);
    OLED_Putc('0' + raw_r, &Font_7x10, OLED_COLOR_WHITE);

    OLED_Update();
}
