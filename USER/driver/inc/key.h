#ifndef __KEY_H
#define __KEY_H

#include <stdint.h>

/**
 * @brief  Initialize debounce state for KEY1/2/3.
 */
void Key_Init(void);

/**
 * @brief  1 ms tick handler — scan KEY1/2/3 with debounce.
 *         Must be called from SysTick_Handler every 1 ms.
 */
void Key_Tick(void);

/**
 * @brief  Query debounced key state.
 * @param  key_id  1 = KEY1, 2 = KEY2, 3 = KEY3
 * @retval 1 = pressed, 0 = released
 */
uint8_t Key_Pressed(uint8_t key_id);

/**
 * @brief  Query rising-edge event (release→press), auto-clears.
 * @param  key_id  1 = KEY1, 2 = KEY2, 3 = KEY3
 * @retval 1 = just pressed this tick, 0 = no event
 */
uint8_t Key_RisingEdge(uint8_t key_id);

#endif /* __KEY_H */
