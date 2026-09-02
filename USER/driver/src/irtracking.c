#include "irtracking.h"
#include "main.h"

/*
 * 四路循迹传感器布局（从小车前进方向看）：
 *
 *   左 ←————————————→ 右
 *        X1    X2    X3    X4
 *
 * 电平约定：检测到黑线 → 低电平(0)，白色地面 → 高电平(1)
 */

/* ========== 初始化 ========== */

void IRTracking_Init(void)
{
    /* GPIO 已由 CubeMX 配置为 Input，无需额外初始化 */
}

/* ========== 读取单路传感器 ========== */

uint8_t IRTracking_Read(uint8_t channel)
{
    switch (channel) {
    case 0: return (HAL_GPIO_ReadPin(IR_X1_GPIO_Port, IR_X1_Pin) == GPIO_PIN_RESET) ? 0 : 1;
    case 1: return (HAL_GPIO_ReadPin(IR_X2_GPIO_Port, IR_X2_Pin) == GPIO_PIN_RESET) ? 0 : 1;
    case 2: return (HAL_GPIO_ReadPin(IR_X3_GPIO_Port, IR_X3_Pin) == GPIO_PIN_RESET) ? 0 : 1;
    case 3: return (HAL_GPIO_ReadPin(IR_X4_GPIO_Port, IR_X4_Pin) == GPIO_PIN_RESET) ? 0 : 1;
    default: return 1;
    }
}

/* ========== 读取全部 4 路 ========== */

uint8_t IRTracking_ReadAll(void)
{
    uint8_t state = 0;
    if (HAL_GPIO_ReadPin(IR_X1_GPIO_Port, IR_X1_Pin) != GPIO_PIN_RESET) state |= 0x01;
    if (HAL_GPIO_ReadPin(IR_X2_GPIO_Port, IR_X2_Pin) != GPIO_PIN_RESET) state |= 0x02;
    if (HAL_GPIO_ReadPin(IR_X3_GPIO_Port, IR_X3_Pin) != GPIO_PIN_RESET) state |= 0x04;
    if (HAL_GPIO_ReadPin(IR_X4_GPIO_Port, IR_X4_Pin) != GPIO_PIN_RESET) state |= 0x08;
    return state;
}

/*
 * ReadAll 返回值速查（bit: X1 X2 X3 X4）：
 *   0b0110 = 0x06 → X2,X3 在黑线，前进
 *   0b1110 = 0x0E → 偏左，右转
 *   0b0010 = 0x02 → 偏右，左转
 *   0b1010 = 0x0A → 大幅偏左，大右转
 *   0b0100 = 0x04 → 大幅偏右，大左转
 *   0b1111 = 0x0F → 全白（脱线）
 *   0b0000 = 0x00 → 全黑（十字路口）
 */
