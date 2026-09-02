#ifndef __TEST_MOTOR_GPIO_H
#define __TEST_MOTOR_GPIO_H

/**
 * @brief  Test motor via direct GPIO (bypass PWM).
 *         Reconfigures motor pins as GPIO Output, then drives
 *         all 4 motors forward: IN1=HIGH, IN2=LOW.
 *         Call once from main after peripheral init.
 */
void TestMotor_GPIO_AllForward(void);

#endif /* __TEST_MOTOR_GPIO_H */
