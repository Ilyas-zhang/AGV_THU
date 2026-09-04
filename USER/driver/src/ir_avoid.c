/*
 * ir_avoid.c — IR obstacle avoidance sensor driver
 *
 * Hardware:
 *   Left:  emitter PE5 (left_infrared), receiver PF9
 *   Right: emitter PE6 (right_infrared), receiver PF10
 *
 * Emitter active-LOW: PE5/PE6 = LOW → emitter ON, HIGH → emitter OFF
 *
 * Active-low convention: receiver LOW = obstacle detected.
 * IRAvoid_ReadLeft/Right() return 1 for obstacle.
 *
 * Asymmetric debounce filter (driven by IRAvoid_Tick @ 1 kHz).
 */

#include "ir_avoid.h"
#include "main.h"

/* ---- Filter state ---- */

static uint8_t  l_detected   = 0;   /* filtered output: 1 = obstacle */
static uint8_t  r_detected   = 0;
static uint8_t  l_clear_cnt  = 0;   /* consecutive clear-read counter */
static uint8_t  r_clear_cnt  = 0;

/* ---- Initialization / shutdown ---- */

void IRAvoid_Init(void)
{
    /* Configure PF9 and PF10 as GPIO input, no pull */
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin   = IRAVOID_L_Pin | IRAVOID_R_Pin;
    GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

    /* Turn on emitters (active-LOW) */
    IRAvoid_EmitterOn();

    /* Reset filter state */
    l_detected  = 0;
    r_detected  = 0;
    l_clear_cnt = 0;
    r_clear_cnt = 0;
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

/* ---- 1 ms tick: sample raw GPIO, update asymmetric debounce ---- */

void IRAvoid_Tick(void)
{
    /* Raw reads (active-low: LOW = obstacle) */
    uint8_t raw_l = (HAL_GPIO_ReadPin(IRAVOID_L_GPIO_Port, IRAVOID_L_Pin) == GPIO_PIN_RESET) ? 1 : 0;
    uint8_t raw_r = (HAL_GPIO_ReadPin(IRAVOID_R_GPIO_Port, IRAVOID_R_Pin) == GPIO_PIN_RESET) ? 1 : 0;

    /* ---- Left channel (baseline) ---- */
    if (raw_l) {
        l_detected  = 1;
        l_clear_cnt = 0;
    } else {
        if (++l_clear_cnt >= IRAVOID_L_RELEASE) {
            l_detected  = 0;
            l_clear_cnt = 0;
        }
    }

    /* ---- Right channel (slower release) ---- */
    if (raw_r) {
        r_detected  = 1;
        r_clear_cnt = 0;
    } else {
        if (++r_clear_cnt >= IRAVOID_R_RELEASE) {
            r_detected  = 0;
            r_clear_cnt = 0;
        }
    }
}

/* ---- Sensor read (returns filtered state, no I/O) ---- */

uint8_t IRAvoid_ReadLeft(void)
{
    return l_detected;
}

uint8_t IRAvoid_ReadRight(void)
{
    return r_detected;
}

uint8_t IRAvoid_ReadAll(void)
{
    uint8_t result = 0;
    if (l_detected) result |= 0x01;  /* bit0 = left */
    if (r_detected) result |= 0x02;  /* bit1 = right */
    return result;
}

/* ---- Emitter control (active-LOW) ---- */

void IRAvoid_EmitterOn(void)
{
    HAL_GPIO_WritePin(left_infrared_GPIO_Port, left_infrared_Pin, GPIO_PIN_RESET);  /* LOW = ON */
    HAL_GPIO_WritePin(right_infrared_GPIO_Port, right_infrared_Pin, GPIO_PIN_RESET);
}

void IRAvoid_EmitterOff(void)
{
    HAL_GPIO_WritePin(left_infrared_GPIO_Port, left_infrared_Pin, GPIO_PIN_SET);    /* HIGH = OFF */
    HAL_GPIO_WritePin(right_infrared_GPIO_Port, right_infrared_Pin, GPIO_PIN_SET);
}
