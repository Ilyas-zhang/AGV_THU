/*
 * line_follow.h — 循迹运动控制（加权误差 + Kp 比例差速 + EMA 滤波）
 *
 * 根据 4 路循迹传感器的位置计算加权误差（连续值），
 * 经 EMA 低通滤波后，通过比例增益 Kp 映射为左右差速，实现平滑转向。
 *
 * 传感器布局（前进方向看，物理顺序）：X2(左) [X1 X3](中) X4(右)
 *   X1/X3 物理间距太近，合并为等效中心传感器 mid = X1∨X3
 *   加权位置: X2=-3, mid=0, X4=+3
 *   error = (-3)*X2 + (+3)*X4
 *
 * 比例差速:
 *   left_speed  = base_speed + Kp * filtered_error
 *   right_speed = base_speed - Kp * filtered_error
 *   快侧 ≥ MIN_SPEED（必须能驱动），慢侧 ≥ 0（允许停转增大差速）
 *
 * 脱线恢复:
 *   全白时按 last_error 方向原地旋转找回
 */

#ifndef __LINE_FOLLOW_H
#define __LINE_FOLLOW_H

#include <stdint.h>

/* ---- 初始化 ---- */

/**
 * @brief  Initialize line-follow driver (IR tracking + motor).
 */
void LineFollow_Init(void);

/* ---- 运行 ---- */

/**
 * @brief  Run one iteration of line-follow control.
 *         Reads sensor state, computes weighted error,
 *         applies EMA filter then proportional differential drive.
 * @param  base_speed  PWM duty 0~3599
 */
void LineFollow_Run(int16_t base_speed);

/* ---- 调试接口 ---- */

/**
 * @brief  Get last raw weighted error value.
 * @retval Weighted error: negative=线偏左(需右转), positive=线偏右(需左转)
 *         Range: -3~+3 (3 effective sensors: X2=-3, mid=0, X4=+3)
 */
int8_t LineFollow_GetError(void);

/**
 * @brief  Get EMA-filtered error value.
 * @retval Filtered error (same units as raw error, smoothed)
 */
int16_t LineFollow_GetFilteredError(void);

#endif /* __LINE_FOLLOW_H */
