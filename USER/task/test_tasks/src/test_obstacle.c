/*
 * test_obstacle.c — Ultrasonic obstacle avoidance test task with OLED
 *
 * OLED 128×32, Font_7x10 layout (~3 lines × ~18 chars):
 *   Line 0: "D: xxx cm  S: FWD"
 *   Line 1: "WARN:30 STOP:15"
 *
 * Calls ObstacleAvoid_Tick() every 1 ms (state machine + trigger).
 * Refreshes OLED when new ultrasonic data arrives.
 */

#include "test_obstacle.h"
#include "obstacle_avoid.h"
#include "ultrasonic.h"
#include "oled.h"

/* ---- display refresh flag ---- */
static uint8_t display_pending = 0;

/* ---- Helper: write uint16_t as decimal at cursor ---- */
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
    /* ObstacleAvoid_Init does NOT call Ultrasonic_Init;
       Ultrasonic_Init is called separately from main.c before this */
    ObstacleAvoid_Init();

    /* Initial OLED display */
    OLED_Clear();
    OLED_GotoXY(0, 0);
    OLED_Puts("D: -- cm  S: ---", &Font_7x10, OLED_COLOR_WHITE);
    OLED_GotoXY(0, 10);
    OLED_Puts("WARN:30 STOP:15", &Font_7x10, OLED_COLOR_WHITE);
    OLED_Update();

    display_pending = 0;
}

void TestObstacle_Tick(void)
{
    /* Drive the avoidance state machine (includes ultrasonic trigger) */
    ObstacleAvoid_Tick();

    /* Check if new data arrived (ObstacleAvoid already consumed it,
       but we can refresh display on every trigger cycle) */
    /* We use a simple approach: check distance change or
       just refresh on every trigger interval (~60 ms) via a counter.
       Since ObstacleAvoid_Tick handles the trigger, we just
       set display_pending when the avoidance state changes or
       new data comes. Simplest: refresh every 60 ms. */
    static uint16_t disp_cnt = 0;
    if (++disp_cnt >= 60) {
        disp_cnt = 0;
        display_pending = 1;
    }

    if (display_pending) {
        display_pending = 0;

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
}
