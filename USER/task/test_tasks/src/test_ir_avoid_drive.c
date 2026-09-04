/*
 * test_ir_avoid_drive.c — IR avoidance driving test with OLED display
 *
 * Drives the IR avoidance state machine and displays sensor + state on OLED.
 *
 * OLED 128×32, Font_7x10 layout:
 *   Line 0: "L:0 R:0 S:FWD   "
 *   Line 1: "IR Avoid Drive  "
 */

#include "test_ir_avoid_drive.h"
#include "ir_avoid_drive.h"
#include "ir_avoid.h"
#include "oled.h"

#define IAD_OLED_REFRESH    100     /* OLED refresh interval ms */

static uint16_t disp_cnt = 0;

void TestIRAvoidDrive_Init(void)
{
    IRAvoid_Init();
    IRAvoidDrive_Init();

    disp_cnt = 0;

    /* Initial OLED display */
    OLED_Clear();
    OLED_GotoXY(0, 0);
    OLED_Puts("L:- R:- S:---  ", &Font_7x10, OLED_COLOR_WHITE);
    OLED_GotoXY(0, 10);
    OLED_Puts("IR Avoid Drive ", &Font_7x10, OLED_COLOR_WHITE);
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

    OLED_Clear();

    /* Line 0: "L:x R:x S:FWD   " */
    OLED_GotoXY(0, 0);
    OLED_Puts("L:", &Font_7x10, OLED_COLOR_WHITE);
    OLED_Putc('0' + left, &Font_7x10, OLED_COLOR_WHITE);
    OLED_Puts(" R:", &Font_7x10, OLED_COLOR_WHITE);
    OLED_Putc('0' + right, &Font_7x10, OLED_COLOR_WHITE);
    OLED_Puts(" S:", &Font_7x10, OLED_COLOR_WHITE);
    OLED_Puts(state_name, &Font_7x10, OLED_COLOR_WHITE);

    /* Line 1: "IR Avoid Drive  " */
    OLED_GotoXY(0, 10);
    OLED_Puts("IR Avoid Drive", &Font_7x10, OLED_COLOR_WHITE);

    OLED_Update();
}
