/*
 * test_obstacle.c — Ultrasonic obstacle avoidance sensor test (display only)
 *
 * Reads ultrasonic distance and avoidance state, displays on OLED.
 * NO motor control — pure sensor reading + display.
 *
 * OLED 128×32, Font_7x10 layout:
 *   Line 0: "D: xxx cm  S: FWD"
 *   Line 1: "WARN:30 STOP:15"
 */

#include "test_obstacle.h"
#include "obstacle_avoid.h"
#include "ultrasonic.h"
#include "oled.h"

#define OA_OLED_REFRESH    100     /* OLED refresh interval ms */

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

void TestObstacle_Init(void)
{
    ObstacleAvoid_Init();

    disp_cnt = 0;

    /* Initial OLED display */
    OLED_Clear();
    OLED_GotoXY(0, 0);
    OLED_Puts("D: -- cm  S: ---", &Font_7x10, OLED_COLOR_WHITE);
    OLED_GotoXY(0, 10);
    OLED_Puts("WARN:30 STOP:15", &Font_7x10, OLED_COLOR_WHITE);
    OLED_Update();
}

void TestObstacle_Tick(void)
{
    /* Drive the avoidance state machine (includes ultrasonic trigger) */
    ObstacleAvoid_Tick();

    if (++disp_cnt < OA_OLED_REFRESH) return;
    disp_cnt = 0;

    uint16_t dist_cm = (ObstacleAvoid_GetDistance() + 5) / 10;
    const char *state_name = ObstacleAvoid_GetStateName();

    OLED_Clear();

    /* Line 0: "D: xxx cm  S: FWD" */
    OLED_GotoXY(0, 0);
    OLED_Puts("D: ", &Font_7x10, OLED_COLOR_WHITE);
    put_uint16(dist_cm, &Font_7x10, OLED_COLOR_WHITE);
    OLED_Puts(" cm  S: ", &Font_7x10, OLED_COLOR_WHITE);
    OLED_Puts(state_name, &Font_7x10, OLED_COLOR_WHITE);

    /* Line 1: "WARN:30 STOP:15" */
    OLED_GotoXY(0, 10);
    OLED_Puts("WARN:30 STOP:15", &Font_7x10, OLED_COLOR_WHITE);

    OLED_Update();
}
