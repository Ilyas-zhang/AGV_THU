/*
 * line_follow.h — 循迹运动控制（加权误差 + 比例差速）
 *
 * 根据 4 路循迹传感器的位置计算加权误差（连续值），
 * 再通过比例增益 Kp 映射为左右差速，实现平滑转向。
 *
 * 相比逐状态 switch-case：
 *   - 代码更简洁，无需枚举 16 种状态
 *   - 转向平滑，无阶跃跳变
 *   - 只调 Kp 一个参数即可改变响应灵敏度
 *
 * 传感器布局（前进方向看，物理顺序）：X2(左) X1 X3 X4(右)
 *   X2 在黑线上 → 误差 = -3（大幅偏右，需左转）
 *   X1 在黑线上 → 误差 = -1（小幅偏右）
 *   X3 在黑线上 → 误差 = +1（小幅偏左）
 *   X4 在黑线上 → 误差 = +3（大幅偏左，需右转）
 */

#ifndef __LINE_FOLLOW_H
#define __LINE_FOLLOW_H

#include <stdint.h>

/* ---- 初始化 ---- */

/**
 * @brief  Initialize line-follow task (IR tracking + motor).
 */
void LineFollow_Init(void);

/* ---- 运行 ---- */

/**
 * @brief  Run one iteration of line-follow control.
 *         Reads sensor state, computes weighted error,
 *         applies proportional differential drive.
 * @param  base_speed  PWM duty 0~3599
 */
void LineFollow_Run(int16_t base_speed);

/* ---- 调试接口 ---- */

/**
 * @brief  Get last computed error value (for OLED display / tuning).
 * @retval Weighted error: negative=偏右, positive=偏左, 0=居中
 */
int8_t LineFollow_GetError(void);

#endif /* __LINE_FOLLOW_H */
