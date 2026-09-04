/*
 * test_ir_avoid_drive.c — IR avoidance driving test with OLED display
 *
 * Drives the IR avoidance state machine and displays sensor + state on OLED.
 *
 * OLED 128×32, Font_7x10 layout:
 *   Line 0: "L:0 R:0 S:FWD   "
 *   Line 1: "LF:0 RF:0 DRV   "
 */

#include "test_ir_avoid_drive.h"
#include "ir_avoid_drive.h"
#include "ir_avoid.h"
#include "oled.h"
#include "main.h"

#define IAD_OLED_REFRESH    100     /* OLED refresh interval ms */

static uint16_t disp_cnt = 0;

void TestIRAvoidDrive_Init(void)
{
    IRAvoid_Init();
    IRAvoidDrive_Init();

    disp_cnt = 0;

    OLED_Clear();
    OLED_GotoXY(0, 0);
    OLED_Puts("L:- R:- S:---  ", &Font_7x10, OLED_COLOR_WHITE);
    OLED_GotoXY(0, 10);
    OLED_Puts("LF:- RF:- DRV  ", &Font_7x10, OLED_COLOR_WHITE);
    OLED_Update();
}

void TestIRAvoidDrive_Tick(void)
{
    /* Drive the avoidance state machine */
    IRAvoidDrive_Tick();

    if (++disp_cnt < IAD_OLED_REFRESH) return;
    disp_cnt = 0;

    uint8_t left  = IRAvoid_ReadLeft();
    uint8_t right = IRAvoid_ReadRight();
    const char *state_name = IRAvoidDrive_GetStateName();

    /* Raw GPIO level for debug */
    uint8_t raw_l = (HAL_GPIO_ReadPin(IRAVOID_L_GPIO_Port, IRAVOID_L_Pin) != GPIO_PIN_RESET) ? 1 : 0;
    uint8_t raw_r = (HAL_GPIO_ReadPin(IRAVOID_R_GPIO_Port, IRAVOID_R_Pin) != GPIO_PIN_RESET) ? 1 : 0;

    OLED_Clear();

    /* Line 0: "L:x R:x S:FWD   " */
    OLED_GotoXY(0, 0);
    OLED_Puts("L:", &Font_7x10, OLED_COLOR_WHITE);
    OLED_Putc('0' + left, &Font_7x10, OLED_COLOR_WHITE);
    OLED_Puts(" R:", &Font_7x10, OLED_COLOR_WHITE);
    OLED_Putc('0' + right, &Font_7x10, OLED_COLOR_WHITE);
    OLED_Puts(" S:", &Font_7x10, OLED_COLOR_WHITE);
    OLED_Puts(state_name, &Font_7x10, OLED_COLOR_WHITE);

    /* Line 1: "LF:x RF:x DRV  " */
    OLED_GotoXY(0, 10);
    OLED_Puts("LF:", &Font_7x10, OLED_COLOR_WHITE);
    OLED_Putc('0' + raw_l, &Font_7x10, OLED_COLOR_WHITE);
    OLED_Puts(" RF:", &Font_7x10, OLED_COLOR_WHITE);
    OLED_Putc('0' + raw_r, &Font_7x10, OLED_COLOR_WHITE);
    OLED_Puts(" DRV", &Font_7x10, OLED_COLOR_WHITE);

    OLED_Update();
}
