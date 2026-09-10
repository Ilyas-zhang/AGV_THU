/*
 * param_tune.h — 按键调参驱动
 *
 * 长按 K1 进入调参模式：
 *   短按 K1 — 切换到下一个参数
 *   短按 K2 — 增加当前参数值 (+step)
 *   短按 K3 — 减小当前参数值 (-step)
 *   长按 K1 — 退出调参模式
 *
 * 进入调参模式时停车，OLED 显示参数名和当前值。
 *
 * 用法：
 *   1. ParamTune_Init()
 *   2. ParamTune_Register("NAME", &var, min, max, step)  注册可调变量
 *   3. ParamTune_Tick()  每次从 1ms Tick 调用
 *   4. ParamTune_IsActive()  判断是否在调参模式（主任务据此跳过电机输出）
 */

#ifndef __PARAM_TUNE_H
#define __PARAM_TUNE_H

#include <stdint.h>

/**
 * @brief  Initialize tuning state. Call before Register.
 */
void ParamTune_Init(void);

/**
 * @brief  1 ms tick — key logic + OLED refresh in tuning mode.
 */
void ParamTune_Tick(void);

/**
 * @brief  Query whether tuning mode is active.
 * @retval 1 = tuning (motors stopped), 0 = normal operation
 */
uint8_t ParamTune_IsActive(void);

/**
 * @brief  Register a tunable parameter.
 * @param  name   Display name (max ~8 chars for OLED 7x10 font)
 * @param  value  Pointer to the runtime variable (must remain in scope)
 * @param  min    Minimum allowed value
 * @param  max    Maximum allowed value
 * @param  step   Increment/decrement step size
 */
void ParamTune_Register(const char *name, int32_t *value,
                         int32_t min, int32_t max, int32_t step);

#endif /* __PARAM_TUNE_H */
