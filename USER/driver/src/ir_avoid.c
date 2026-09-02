/*
 * ir_avoid.c — IR obstacle avoidance sensor driver
 *
 * Hardware:
 *   Left:  emitter PE5 (left_infrared), receiver PF9
 *   Right: emitter PE6 (right_infrared), receiver PF10
 *
 * Active-low convention: receiver LOW = obstacle detected.
 * IRAvoid_ReadLeft/Right() return 1 for obstacle (inverted for caller convenience).
 */

#include "ir_avoid.h"
#include "main.h"

/* ---- Initialization / shutdown ---- */

void IRAvoid_Init(void)
{
    /* Configure PF9 and PF10 as GPIO input, no pull */
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin   = IRAVOID_L_Pin | IRAVOID_R_Pin;
    GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

    /* Turn on emitters */
    IRAvoid_EmitterOn();
}

void IRAvoid_DeInit(void)
{
    /* Turn off emitters */
    IRAvoid_EmitterOff();

    /* Reset receiver pins to analog (lowest power) */
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin   = IRAVOID_L_Pin | IRAVOID_R_Pin;
    GPIO_InitStruct.Mode  = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);
}

/* ---- Sensor read ---- */

uint8_t IRAvoid_ReadLeft(void)
{
    /* LOW = obstacle → return 1 */
    return (HAL_GPIO_ReadPin(IRAVOID_L_GPIO_Port, IRAVOID_L_Pin) == GPIO_PIN_RESET) ? 1 : 0;
}

uint8_t IRAvoid_ReadRight(void)
{
    return (HAL_GPIO_ReadPin(IRAVOID_R_GPIO_Port, IRAVOID_R_Pin) == GPIO_PIN_RESET) ? 1 : 0;
}

uint8_t IRAvoid_ReadAll(void)
{
    uint8_t result = 0;
    if (IRAvoid_ReadLeft())  result |= 0x01;  /* bit0 = left */
    if (IRAvoid_ReadRight()) result |= 0x02;  /* bit1 = right */
    return result;
}

/* ---- Emitter control ---- */

void IRAvoid_EmitterOn(void)
{
    HAL_GPIO_WritePin(left_infrared_GPIO_Port, left_infrared_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(right_infrared_GPIO_Port, right_infrared_Pin, GPIO_PIN_SET);
}

void IRAvoid_EmitterOff(void)
{
    HAL_GPIO_WritePin(left_infrared_GPIO_Port, left_infrared_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(right_infrared_GPIO_Port, right_infrared_Pin, GPIO_PIN_RESET);
}
