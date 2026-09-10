#ifndef __LED_CONFIG_H
#define __LED_CONFIG_H

/*
 * LED 驱动参数配置
 *
 * 软件 PWM 频率 = 1000 / PWM_PERIOD (Hz)
 * 慢闪烁频率 = 1000 / (2 × BLINK_SLOW_HALF) (Hz)
 * 快闪烁频率 = 1000 / (2 × BLINK_FAST_HALF) (Hz)
 */

/* ---- 软件 PWM ---- */
#define LED_PWM_PERIOD           100     /* PWM 步数, 100 × 1ms = 100Hz */

/* ---- 闪烁半周期 (ms) ---- */
#define LED_BLINK_SLOW_HALF      500     /* 500ms → 1Hz 慢闪烁 */
#define LED_BLINK_FAST_HALF      100     /* 100ms → 5Hz 快闪烁 */

/* ---- 预设颜色强度 (0~100) ---- */
#define L_GREEN                  100     /* 左绿 */
#define R_GREEN                  100     /* 右绿 */
#define L_YELLOW_R               100     /* 左黄-红分量 */
#define L_YELLOW_G               80      /* 左黄-绿分量 */
#define R_YELLOW_R               100     /* 右黄-红分量 */
#define R_YELLOW_G               80      /* 右黄-绿分量 */

/* ---- 闪烁模式 ---- */
#define BLINK_OFF                0       /* 常亮 */
#define BLINK_SLOW               1       /* 慢闪 1Hz */
#define BLINK_FAST               2       /* 快闪 5Hz */

/* ---- 车辆状态预设 ---- */
#define LED_PRESET_FORWARD       0, L_GREEN, 0, BLINK_OFF,  0, R_GREEN, 0, BLINK_OFF
#define LED_PRESET_STOP          100, 0, 0, BLINK_FAST,  100, 0, 0, BLINK_FAST
#define LED_PRESET_BACKWARD      L_YELLOW_R, L_YELLOW_G, 0, BLINK_OFF,  R_YELLOW_R, R_YELLOW_G, 0, BLINK_OFF
#define LED_PRESET_TURN_LEFT     0, L_GREEN, 0, BLINK_OFF,  R_YELLOW_R, R_YELLOW_G, 0, BLINK_SLOW
#define LED_PRESET_TURN_RIGHT    L_YELLOW_R, L_YELLOW_G, 0, BLINK_SLOW,  0, R_GREEN, 0, BLINK_OFF
#define LED_PRESET_ROTATE        100, 0, 0, BLINK_SLOW,   0, R_GREEN, 0, BLINK_SLOW

#endif /* __LED_CONFIG_H */
