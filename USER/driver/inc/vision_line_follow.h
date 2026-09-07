/*
 * vision_line_follow.h — 视觉循迹驱动（K210 摄像头 + 比例差速）
 *
 * K210 摄像头检测黑线位置，通过 UART 发送误差值：
 *   $e<value>#  value: -100~+100 (负=偏左, 正=偏右), 999=脱线
 *
 * STM32 接收后经 EMA 滤波，做 Kp 比例差速：
 *   left_speed  = base + Kp * filtered
 *   right_speed = base - Kp * filtered
 *
 * 脱线/超时 → 按上次误差方向旋转找回
 */

#ifndef __VISION_LINE_FOLLOW_H
#define __VISION_LINE_FOLLOW_H

#include <stdint.h>

/* ---- 初始化 ---- */

/**
 * @brief  初始化视觉循迹驱动 (K210 通讯 + 电机)。
 */
void VisionLineFollow_Init(void);

/* ---- 周期运行 ---- */

/**
 * @brief  1 ms tick — 处理 K210 消息 + EMA 滤波 + 差速控制。
 *         从 SysTick 调用。
 */
void VisionLineFollow_Tick(void);

/* ---- 调试接口 ---- */

/**
 * @brief  获取 EMA 滤波后的误差值。
 * @retval 滤波误差 (与原始同量纲, ≈ -100~+100)
 */
int16_t VisionLineFollow_GetFilteredError(void);

/**
 * @brief  获取最近一次原始误差值。
 * @retval 原始误差 (-100~+100, 或 999=脱线)
 */
int16_t VisionLineFollow_GetRawError(void);

/**
 * @brief  是否脱线（未找到黑线或通讯超时）。
 * @retval 1=脱线, 0=在线
 */
uint8_t VisionLineFollow_IsOffline(void);

#endif /* __VISION_LINE_FOLLOW_H */
