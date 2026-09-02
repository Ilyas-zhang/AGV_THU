#ifndef __TEST_BUZZ_H
#define __TEST_BUZZ_H

/**
 * @brief  Initialize buzzer test: KEY1 press → buzz on, KEY2 press → buzz off.
 */
void TestBuzz_Init(void);

/**
 * @brief  1 ms tick handler for buzzer test.
 *         Must be called from SysTick_Handler every 1 ms.
 */
void TestBuzz_Tick(void);

#endif /* __TEST_BUZZ_H */
