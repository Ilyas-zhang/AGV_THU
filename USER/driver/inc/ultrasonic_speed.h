/*
 * ultrasonic_speed.h — Speed estimation from ultrasonic distance readings
 *
 * Computes velocity = Δdistance / Δtime using DWT timestamps.
 * Applies exponential moving average (EMA) filter for smoothing.
 *
 * Positive speed = approaching obstacle (distance decreasing)
 * Negative speed = moving away (distance increasing)
 */

#ifndef __ULTRASONIC_SPEED_H
#define __ULTRASONIC_SPEED_H

#include <stdint.h>

/**
 * @brief  Initialize speed estimator. Call once after Ultrasonic_Init().
 */
void UltrasonicSpeed_Init(void);

/**
 * @brief  Feed a new distance sample (call when Ultrasonic_IsReady() returns 1).
 * @param  distance_mm  Current distance in mm from Ultrasonic_GetDistance().
 */
void UltrasonicSpeed_Update(uint16_t distance_mm);

/**
 * @brief  Get filtered speed in cm/s (signed).
 * @retval Positive = approaching, Negative = receding.
 */
int16_t UltrasonicSpeed_GetCmPerS(void);

/**
 * @brief  Get raw (unfiltered) speed in cm/s from last update.
 */
int16_t UltrasonicSpeed_GetRawCmPerS(void);

#endif /* __ULTRASONIC_SPEED_H */
