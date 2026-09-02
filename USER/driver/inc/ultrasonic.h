/*
 * ultrasonic.h — HC-SR04 ultrasonic ranging driver
 *
 * Trigger (PF11): output, send 10 μs pulse
 * Echo   (PF12): input with EXTI, measures round-trip time
 *
 * Non-blocking, interrupt-driven (DWT cycle counter + EXTI).
 * Call Ultrasonic_Trigger() periodically (≥60 ms apart),
 * then read distance with Ultrasonic_GetDistance().
 */

#ifndef __ULTRASONIC_H
#define __ULTRASONIC_H

#include <stdint.h>

/* ---- Initialization ---- */

/**
 * @brief  Initialize ultrasonic driver.
 *         Enables DWT cycle counter, reconfigures Echo pin as EXTI,
 *         sets Trigger pin LOW.
 */
void Ultrasonic_Init(void);

/* ---- Trigger ---- */

/**
 * @brief  Send a 10 μs trigger pulse to start a measurement.
 *         Call periodically (e.g. every 60 ms from main loop or SysTick).
 *         Distance result is updated on Echo falling edge via EXTI.
 */
void Ultrasonic_Trigger(void);

/* ---- Read result ---- */

/**
 * @brief  Get the last measured distance in mm.
 * @retval Distance in mm. 0 = no echo or out of range.
 */
uint16_t Ultrasonic_GetDistance(void);

/**
 * @brief  Get the last measured distance in cm (integer).
 * @retval Distance in cm. 0 = no echo or out of range.
 */
uint16_t Ultrasonic_GetDistanceCm(void);

/**
 * @brief  Check if a new measurement is available since last read.
 * @retval 1 = new data ready, 0 = not ready (still waiting or already read).
 *         Calling this clears the ready flag.
 */
uint8_t Ultrasonic_IsReady(void);

/* ---- EXTI handler ---- */

/**
 * @brief  EXTI callback — call from EXTI15_10_IRQHandler when Echo pin fires.
 *         Handles both rising and falling edges.
 */
void Ultrasonic_EXTI_Handler(void);

#endif /* __ULTRASONIC_H */
