/*
 * param_tune.c — 按键调参驱动
 *
 * 长按 K1 进入调参模式，短按 K1 切换参数，K2+/K3-，再长按 K1 退出。
 * 进入时停车，OLED 显示参数名和当前值。
 *
 * 长/短按 K1 判定：
 *   按住 ≥ PT_LONG_PRESS_MS → 立即触发长按动作
 *   松开时未达阈值 → 触发短按动作
 */

#include "param_tune.h"
#include "param_tune_config.h"
#include "key.h"
#include "motor.h"
#include "oled.h"
#include <stdio.h>

/* ---- parameter table ---- */

typedef struct {
    const char *name;       /* display name */
    int32_t    *value;      /* pointer to runtime variable */
    int32_t     min;        /* minimum value */
    int32_t     max;        /* maximum value */
    int32_t     step;       /* increment step */
} PtParam;

static PtParam  pt_params[PT_MAX_PARAMS];
static uint8_t  pt_count   = 0;     /* registered param count */
static uint8_t  pt_active  = 0;     /* 1 = tuning mode */
static uint8_t  pt_sel     = 0;     /* selected param index */

/* ---- K1 long/short press ---- */

static uint16_t k1_hold_ms    = 0;  /* K1 持续按住时间 */
static uint8_t  k1_long_fired = 0;  /* 本次按压已触发长按 */

/* ---- OLED refresh ---- */

static uint16_t disp_cnt = 0;       /* OLED 刷新计时器 */

/* ---- public API ---- */

void ParamTune_Init(void)
{
    pt_count   = 0;
    pt_active  = 0;
    pt_sel     = 0;
    k1_hold_ms    = 0;
    k1_long_fired = 0;
    disp_cnt      = 0;
}

void ParamTune_Register(const char *name, int32_t *value,
                          int32_t min, int32_t max, int32_t step)
{
    if (pt_count >= PT_MAX_PARAMS) return;
    pt_params[pt_count].name  = name;
    pt_params[pt_count].value = value;
    pt_params[pt_count].min   = min;
    pt_params[pt_count].max   = max;
    pt_params[pt_count].step  = step;
    pt_count++;
}

uint8_t ParamTune_IsActive(void)
{
    return pt_active;
}

void ParamTune_Tick(void)
{
    if (pt_count == 0) return;   /* no params registered, skip */

    if (!pt_active) {
        /* ---- IDLE: detect K1 long press to enter tuning ---- */
        if (Key_Pressed(1)) {
            if (++k1_hold_ms >= PT_LONG_PRESS_MS && !k1_long_fired) {
                k1_long_fired = 1;
                pt_active = 1;
                pt_sel = 0;
                pwm_car_stop();         /* 停车 */
            }
        } else {
            k1_hold_ms    = 0;
            k1_long_fired = 0;
        }
    } else {
        /* ---- TUNING mode ---- */

        /* K1 long/short press detection */
        if (Key_Pressed(1)) {
            if (++k1_hold_ms >= PT_LONG_PRESS_MS && !k1_long_fired) {
                k1_long_fired = 1;
                pt_active = 0;          /* 退出调参模式 */
            }
        } else {
            /* K1 just released */
            if (!k1_long_fired) {
                /* 短按 K1: 切换到下一个参数 */
                pt_sel = (pt_sel + 1) % pt_count;
            }
            k1_hold_ms    = 0;
            k1_long_fired = 0;
        }

        /* K2: increase value */
        if (Key_RisingEdge(2)) {
            PtParam *p = &pt_params[pt_sel];
            int32_t new_val = *p->value + p->step;
            if (new_val <= p->max) *p->value = new_val;
            else                   *p->value = p->max;
        }

        /* K3: decrease value */
        if (Key_RisingEdge(3)) {
            PtParam *p = &pt_params[pt_sel];
            int32_t new_val = *p->value - p->step;
            if (new_val >= p->min) *p->value = new_val;
            else                   *p->value = p->min;
        }

        /* ---- OLED display ---- */
        if (++disp_cnt >= PT_DISPLAY_MS) {
            disp_cnt = 0;

            PtParam *p = &pt_params[pt_sel];
            char buf[17];   /* 16 chars + null for 128px / 7px font */

            OLED_Clear();

            /* Row 0: TUNE: NAME */
            OLED_GotoXY(0, 0);
            snprintf(buf, sizeof(buf), "TUNE:%s", p->name);
            OLED_Puts(buf, &Font_7x10, OLED_COLOR_WHITE);

            /* Row 1: VAL: xxx */
            OLED_GotoXY(0, 10);
            snprintf(buf, sizeof(buf), "VAL:%ld", (long)*p->value);
            OLED_Puts(buf, &Font_7x10, OLED_COLOR_WHITE);

            /* Row 2: K1:Nxt K2:+ K3:- */
            OLED_GotoXY(0, 20);
            OLED_Puts("K1:Nxt K2:+ K3:-", &Font_7x10, OLED_COLOR_WHITE);

            OLED_Update();
        }
    }
}
