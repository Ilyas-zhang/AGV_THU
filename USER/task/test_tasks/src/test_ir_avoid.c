/*
 * test_ir_avoid.c — IR obstacle avoidance sensor test (display + LED)
 *
 * Reads left/right IR avoidance sensors and displays on OLED.
 * LED feedback: obstacle → corresponding side red fast blink.
 * Emitter active-LOW: E=0 means ON, E=1 means OFF.
 * No motor control.
 *
 * OLED 128×32, Font_7x10 layout:
 *   Line 0: "L:x R:x  IR Avoid"
 *   Line 1: "E:x x L:x R:x"    (emitter read-back + receiver raw)
 */

#include "test_ir_avoid.h"
#include "ir_avoid.h"
#include "oled.h"
#include "led.h"
#include "led_config.h"
#include "main.h"

#define IRA_OLED_REFRESH    100     /* OLED refresh interval ms */

static uint16_t disp_cnt = 0;

void TestIRAvoid_Init(void)
{
    IRAvoid_Init();

    disp_cnt = 0;

    LED_Set(0, 0, 0, BLINK_OFF, 0, 0, 0, BLINK_OFF);

    OLED_Clear();
    OLED_GotoXY(0, 0);
    OLED_Puts("L:- R:-  IR Avoid", &Font_7x10, OLED_COLOR_WHITE);
    OLED_GotoXY(0, 10);
    OLED_Puts("E:- - L:- R:-   ", &Font_7x10, OLED_COLOR_WHITE);
    OLED_Update();
}

void TestIRAvoid_Tick(void)
{
    /* Read sensors (1 = obstacle, 0 = clear) */
    uint8_t left  = IRAvoid_ReadLeft();
    uint8_t right = IRAvoid_ReadRight();

    /* LED feedback: obstacle → corresponding side red fast blink */
    LED_Set(left  ? 100 : 0, 0, 0, left  ? BLINK_FAST : BLINK_OFF,
            right ? 100 : 0, 0, 0, right ? BLINK_FAST : BLINK_OFF);

    if (++disp_cnt < IRA_OLED_REFRESH) return;
    disp_cnt = 0;

    /* Raw GPIO level */
    uint8_t raw_l = (HAL_GPIO_ReadPin(IRAVOID_L_GPIO_Port, IRAVOID_L_Pin) != GPIO_PIN_RESET) ? 1 : 0;
    uint8_t raw_r = (HAL_GPIO_ReadPin(IRAVOID_R_GPIO_Port, IRAVOID_R_Pin) != GPIO_PIN_RESET) ? 1 : 0;

    /* Emitter read-back (active-LOW: 0=ON, 1=OFF) */
    uint8_t em_l = (HAL_GPIO_ReadPin(left_infrared_GPIO_Port, left_infrared_Pin) != GPIO_PIN_RESET) ? 1 : 0;
    uint8_t em_r = (HAL_GPIO_ReadPin(right_infrared_GPIO_Port, right_infrared_Pin) != GPIO_PIN_RESET) ? 1 : 0;

    OLED_Clear();

    /* Line 0: "L:x R:x  IR Avoid" */
    OLED_GotoXY(0, 0);
    OLED_Puts("L:", &Font_7x10, OLED_COLOR_WHITE);
    OLED_Putc('0' + left, &Font_7x10, OLED_COLOR_WHITE);
    OLED_Puts(" R:", &Font_7x10, OLED_COLOR_WHITE);
    OLED_Putc('0' + right, &Font_7x10, OLED_COLOR_WHITE);
    OLED_Puts("  IR Avoid", &Font_7x10, OLED_COLOR_WHITE);

    /* Line 1: "E:x x L:x R:x"  (E: 0=ON, 1=OFF) */
    OLED_GotoXY(0, 10);
    OLED_Puts("E:", &Font_7x10, OLED_COLOR_WHITE);
    OLED_Putc('0' + em_l, &Font_7x10, OLED_COLOR_WHITE);
    OLED_Putc(' ', &Font_7x10, OLED_COLOR_WHITE);
    OLED_Putc('0' + em_r, &Font_7x10, OLED_COLOR_WHITE);
    OLED_Puts(" L:", &Font_7x10, OLED_COLOR_WHITE);
    OLED_Putc('0' + raw_l, &Font_7x10, OLED_COLOR_WHITE);
    OLED_Puts(" R:", &Font_7x10, OLED_COLOR_WHITE);
    OLED_Putc('0' + raw_r, &Font_7x10, OLED_COLOR_WHITE);

    OLED_Update();
}
