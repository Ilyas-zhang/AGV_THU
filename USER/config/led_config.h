#ifndef __LED_CONFIG_H
#define __LED_CONFIG_H

/*
 * LED_Set 八参数配置别名
 *
 * LED_Set(l_r, l_g, l_b, l_blink, r_r, r_g, r_b, r_blink)
 *   l_r/l_g/l_b  : 左侧 RGB 强度 0~100
 *   l_blink       : 0=稳态, 1=慢闪(1Hz), 2=快闪/爆闪(5Hz)
 *   r_r/r_g/r_b  : 右侧 RGB 强度 0~100
 *   r_blink       : 同上
 */

/* ---- 闪烁模式 ---- */
#define BLINK_OFF   0    /* 稳态 */
#define BLINK_SLOW  1    /* 慢闪 1Hz  (转向灯) */
#define BLINK_FAST  2    /* 爆闪 5Hz  (停车灯/倒车灯) */

/* ---- 基础亮度 ---- */
#define L_RED       100
#define L_GREEN     100
#define L_BLUE      100
#define R_RED       100
#define R_GREEN     100
#define R_BLUE      100

/* ---- 黄色 = 红+绿 (RGB 混色) ---- */
#define L_YELLOW_R  100
#define L_YELLOW_G  100
#define R_YELLOW_R  100
#define R_YELLOW_G  100

/* ========== 车辆灯效预设 ========== */

/* 直行：双绿稳态 */
#define LED_PRESET_FORWARD \
    0, L_GREEN, 0, BLINK_OFF, \
    0, R_GREEN, 0, BLINK_OFF

/* 停车：双红爆闪 */
#define LED_PRESET_STOP \
    L_RED, 0, 0, BLINK_FAST, \
    R_RED, 0, 0, BLINK_FAST

/* 倒车：双黄爆闪 */
#define LED_PRESET_BACKWARD \
    L_YELLOW_R, L_YELLOW_G, 0, BLINK_FAST, \
    R_YELLOW_R, R_YELLOW_G, 0, BLINK_FAST

/* 左转：左黄慢闪, 右绿稳态 */
#define LED_PRESET_TURN_LEFT \
    L_YELLOW_R, L_YELLOW_G, 0, BLINK_SLOW, \
    0, R_GREEN, 0, BLINK_OFF

/* 右转：左绿稳态, 右黄慢闪 */
#define LED_PRESET_TURN_RIGHT \
    0, L_GREEN, 0, BLINK_OFF, \
    R_YELLOW_R, R_YELLOW_G, 0, BLINK_SLOW

/* 原地旋转：双黄慢闪 */
#define LED_PRESET_ROTATE \
    L_YELLOW_R, L_YELLOW_G, 0, BLINK_SLOW, \
    R_YELLOW_R, R_YELLOW_G, 0, BLINK_SLOW

/* ========== 通用预设 ========== */

/* 双绿稳态 (同 FORWARD) */
#define LED_PRESET_BOTH_GREEN  LED_PRESET_FORWARD

/* 全灭 */
#define LED_PRESET_ALL_OFF \
    0, 0, 0, BLINK_OFF, \
    0, 0, 0, BLINK_OFF

#endif /* __LED_CONFIG_H */
