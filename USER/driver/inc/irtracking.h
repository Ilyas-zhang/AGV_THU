/*
 * irtracking.h — 四路红外循迹传感器驱动
 *
 * PF13/PF14/PF15/PG0 (X1~X4), active-low: LOW = 黑线, HIGH = 白地
 *
 * 带非对称去抖滤波（IRTracking_Tick @ 1 kHz 驱动）：
 *   - 检测到黑线（LOW）：立即触发
 *   - 检测到白地（HIGH）：需连续 N 次才释放
 *   防止接收管释放慢导致的"stuck"问题。
 */

#ifndef __IRTRACKING_H
#define __IRTRACKING_H

#include <stdint.h>
#include "line_follow_config.h"

/* ---- 去抖滤波参数 ---- */

/**
 * 传感器从黑线移到白地时，需连续 IRTRACK_RELEASE 次读到 HIGH
 * 才认为离开了黑线。值越大滤波越强，但响应越慢。
 * 典型值：2~10（对应 2~10 ms at 1 kHz Tick）
 * 参数统一在 line_follow_config.h 中配置
 */

/* ---- 初始化 ---- */

/**
 * @brief  Initialize IR tracking sensor driver.
 *         GPIO is configured by CubeMX; resets filter state.
 */
void IRTracking_Init(void);

/* ---- 周期采样 ---- */

/**
 * @brief  1 ms tick — call from SysTick.
 *         Samples raw GPIO for all 4 channels and updates debounce filter.
 */
void IRTracking_Tick(void);

/* ---- 读取（返回滤波后状态） ---- */

/**
 * @brief  Read a single sensor channel (filtered).
 * @param  channel  0~3 corresponding to X1~X4 (left to right)
 * @retval 0 = black line detected, 1 = white floor
 */
uint8_t IRTracking_Read(uint8_t channel);

/**
 * @brief  Read all 4 channels as a packed 4-bit state (filtered).
 * @retval bit0=X1, bit1=X2, bit2=X3, bit3=X4
 *         0 = black line, 1 = white floor
 */
uint8_t IRTracking_ReadAll(void);

#endif /* __IRTRACKING_H */
