/*
 * ir_avoid.h — IR obstacle avoidance sensor driver
 *
 * Two-channel onboard IR avoidance circuit:
 *   Left:  emitter PE5 (left_infrared), receiver PF9
 *   Right: emitter PE6 (right_infrared), receiver PF10
 *
 * Logic: emitter ON → obstacle reflects IR → receiver pulled LOW
 *   → LOW = obstacle detected, HIGH = clear path
 *   (active-low, same convention as IR line-tracking sensors)
 *
 * PF9/PF10 are NOT configured in CubeMX — IRAvoid_Init() sets them up.
 */

#ifndef __IR_AVOID_H
#define __IR_AVOID_H

#include <stdint.h>
#include "ir_avoid_config.h"

/* ---- Receiver pins (not in CubeMX, defined manually) ---- */
#define IRAVOID_L_GPIO_Port    GPIOF
#define IRAVOID_L_Pin         GPIO_PIN_9

#define IRAVOID_R_GPIO_Port    GPIOF
#define IRAVOID_R_Pin         GPIO_PIN_10

/* ---- Asymmetric debounce filter ---- */

/**
 * IR sensors can get "stuck HIGH" after detecting an obstacle — the
 * receiver doesn't release even after the obstacle leaves.
 *
 * Solution: asymmetric debounce in software.
 *   - Trigger  (obstacle appears): immediate, 1 raw read.
 *   - Release  (obstacle clears) : requires N consecutive clear reads
 *     before we declare the path clear. This filters out the stuck-high
 *     glitch while still responding to genuine clearance.
 *
 * Left  is the baseline (release after 1 clear read = immediate).
 * Right needs IRAVOID_R_RELEASE consecutive clear reads (10 ms at 1 kHz tick).
 *
 * IRAvoid_Tick() must be called every 1 ms from SysTick to drive the filter.
 *
 * Tunable values moved to ir_avoid_config.h.
 */

/* ---- Initialization / shutdown ---- */

/**
 * @brief  Initialize IR avoidance sensor.
 *         Configures PF9/PF10 as GPIO input, turns on emitters (PE5/PE6 HIGH).
 */
void IRAvoid_Init(void);

/**
 * @brief  De-initialize: turn off emitters, reset receiver pins to analog.
 */
void IRAvoid_DeInit(void);

/**
 * @brief  1 ms tick — call from SysTick.
 *         Samples raw GPIO and updates the asymmetric debounce filter.
 */
void IRAvoid_Tick(void);

/* ---- Sensor read ---- */

/**
 * @brief  Read left obstacle sensor.
 * @retval 1 = obstacle detected (receiver LOW), 0 = clear path
 */
uint8_t IRAvoid_ReadLeft(void);

/**
 * @brief  Read right obstacle sensor.
 * @retval 1 = obstacle detected, 0 = clear
 */
uint8_t IRAvoid_ReadRight(void);

/**
 * @brief  Read both sensors as bitmask.
 * @retval bit0 = left (1=obstacle), bit1 = right (1=obstacle)
 */
uint8_t IRAvoid_ReadAll(void);

/* ---- Emitter control ---- */

/**
 * @brief  Turn on both IR emitters (PE5/PE6 = HIGH).
 */
void IRAvoid_EmitterOn(void);

/**
 * @brief  Turn off both IR emitters (PE5/PE6 = LOW).
 */
void IRAvoid_EmitterOff(void);

#endif /* __IR_AVOID_H */
