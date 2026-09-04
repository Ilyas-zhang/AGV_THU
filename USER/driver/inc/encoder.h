/*
 * encoder.h — 四路正交编码器驱动
 *
 * TIM2/3/4/5 Encoder Interface，中点计数法 (CNT = 0x7FFF)。
 * Encoder_Tick() 需在 SysTick 1 kHz 驱动，每次读取增量并累加总计。
 *
 * Timer → Encoder 映射（与硬件接线对应）：
 *   Encoder1 → TIM4 (PD12/PD13)
 *   Encoder2 → TIM2 (PA15/PB3)
 *   Encoder3 → TIM5 (PA0/PA1)
 *   Encoder4 → TIM3 (PB4/PB5, partial remap)
 */

#ifndef __ENCODER_H
#define __ENCODER_H

#include <stdint.h>

/* ---- 初始化 ---- */

/**
 * @brief  启动所有编码器定时器，CNT 置中点 0x7FFF。
 *         需在 MX_TIM2/3/4/5_Init() 之后调用。
 */
void Encoder_Init(void);

/* ---- 周期采样 ---- */

/**
 * @brief  读取四路增量，累加总计，更新速度。
 *         需在 SysTick 1 ms 驱动。
 */
void Encoder_Tick(void);

/* ---- 读取接口 ---- */

/**
 * @brief  获取上次 Tick 以来的增量计数。
 * @param  id: 编码器编号 1~4
 * @retval 有符号增量：正值=正转，负值=反转
 */
int16_t Encoder_GetDelta(uint8_t id);

/**
 * @brief  获取开机以来的累计计数。
 * @param  id: 编码器编号 1~4
 * @retval 累计计数（有符号）
 */
int32_t Encoder_GetTotal(uint8_t id);

/**
 * @brief  获取编码器速度（counts / Tick 间隔）。
 *         Tick 间隔为 1 ms 时，单位为 counts/ms。
 * @param  id: 编码器编号 1~4
 * @retval 速度值（有符号）
 */
int16_t Encoder_GetSpeed(uint8_t id);

/**
 * @brief  一次读出四路增量。
 * @param  deltas: 输出数组，至少 4 个元素，deltas[0~3] 对应 Encoder1~4
 */
void Encoder_GetAllDeltas(int16_t *deltas);

/**
 * @brief  清零所有累计值和增量。
 */
void Encoder_Reset(void);

#endif /* __ENCODER_H */
