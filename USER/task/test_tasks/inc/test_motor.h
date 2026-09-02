#ifndef __TEST_MOTOR_H
#define __TEST_MOTOR_H

/**
 * @brief  Initialize motor test: KEY3 cycles through motion states.
 */
void TestMotor_Init(void);

/**
 * @brief  1 ms tick handler for motor test.
 *         Each KEY3 press cycles: Forward → Backward → Turn Left → Turn Right
 *         → Rotate Left → Rotate Right → Stop → Forward ...
 *         Must be called from SysTick_Handler every 1 ms.
 */
void TestMotor_Tick(void);

#endif /* __TEST_MOTOR_H */
