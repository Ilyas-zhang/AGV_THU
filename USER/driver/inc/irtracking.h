#ifndef __IRTRACKING_H
#define __IRTRACKING_H

#include <stdint.h>

/**
 * @brief  Initialize IR tracking sensor driver.
 *         GPIO is configured by CubeMX; this is a placeholder.
 */
void IRTracking_Init(void);

/**
 * @brief  Read a single sensor channel.
 * @param  channel  0~3 corresponding to X1~X4 (left to right)
 * @retval 0 = black line detected, 1 = white floor
 */
uint8_t IRTracking_Read(uint8_t channel);

/**
 * @brief  Read all 4 channels as a packed 4-bit state.
 * @retval bit0=X1, bit1=X2, bit2=X3, bit3=X4
 *         0 = black line, 1 = white floor
 */
uint8_t IRTracking_ReadAll(void);

#endif /* __IRTRACKING_H */
