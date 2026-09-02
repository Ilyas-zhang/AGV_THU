#include "test_motor_gpio.h"
#include "main.h"

/**
 * @brief  Reconfigure all motor pins as GPIO Push-Pull Output,
 *         then set all motors forward (IN1=HIGH, IN2=LOW).
 *
 *  Motor1: PE9=IN1, PE11=IN2
 *  Motor2: PE13=IN1, PE14=IN2
 *  Motor3: PC6=IN1, PC7=IN2
 *  Motor4: PC8=IN1, PC9=IN2
 */
void TestMotor_GPIO_AllForward(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* ---- PE: Motor1_IN1(PE9), Motor1_IN2(PE11), Motor2_IN1(PE13), Motor2_IN2(PE14) ---- */
    __HAL_RCC_GPIOE_CLK_ENABLE();
    GPIO_InitStruct.Pin   = Motor1_IN1_Pin | Motor1_IN2_Pin
                          | Motor2_IN1_Pin | Motor2_IN2_Pin;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

    /* ---- PC: Motor3_IN1(PC6), Motor3_IN2(PC7), Motor4_IN1(PC8), Motor4_IN2(PC9) ---- */
    __HAL_RCC_GPIOC_CLK_ENABLE();
    GPIO_InitStruct.Pin   = Motor3_IN1_Pin | Motor3_IN2_Pin
                          | Motor4_IN1_Pin | Motor4_IN2_Pin;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /* ---- 全部正转: IN1=HIGH, IN2=LOW ---- */
    HAL_GPIO_WritePin(Motor1_IN1_GPIO_Port, Motor1_IN1_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(Motor1_IN2_GPIO_Port, Motor1_IN2_Pin, GPIO_PIN_RESET);

    HAL_GPIO_WritePin(Motor2_IN1_GPIO_Port, Motor2_IN1_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(Motor2_IN2_GPIO_Port, Motor2_IN2_Pin, GPIO_PIN_RESET);

    HAL_GPIO_WritePin(Motor3_IN1_GPIO_Port, Motor3_IN1_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(Motor3_IN2_GPIO_Port, Motor3_IN2_Pin, GPIO_PIN_RESET);

    HAL_GPIO_WritePin(Motor4_IN1_GPIO_Port, Motor4_IN1_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(Motor4_IN2_GPIO_Port, Motor4_IN2_Pin, GPIO_PIN_RESET);
}
