/*
 * ultrasonic_overtake.h — Ultrasonic obstacle overtaking driver
 *
 * State-machine driven overtaking behavior:
 *   FORWARD → STOP(1s) → LEFT_SHIFT → PASS → RIGHT_SHIFT → FORWARD
 *
 * Uses the existing non-blocking Ultrasonic_* API (DWT + EXTI).
 * Call UltrasonicOvertake_Tick() from SysTick every 1 ms.
 */

#ifndef __ULTRASONIC_OVERTAKE_H
#define __ULTRASONIC_OVERTAKE_H

#include <stdint.h>
#include "ultrasonic_overtake_config.h"

/* ---- Initialization ---- */

/**
 * @brief  Initialize overtaking state machine.
 *         Resets state to FORWARD, clears timers.
 *         Does NOT call Ultrasonic_Init() — caller must do that.
 */
void UltrasonicOvertake_Init(void);

/**
 * @brief  1 ms tick handler — call from SysTick.
 *         Periodically triggers ultrasonic measurement and
 *         drives the overtaking state machine.
 */
void UltrasonicOvertake_Tick(void);

/**
 * @brief  Get current state name string (for display).
 * @retval Short state name: "FWD", "STOP", "LSFT", "PASS", "RSFT"
 */
const char *UltrasonicOvertake_GetStateName(void);

/**
 * @brief  Get current distance in mm (last ultrasonic reading).
 * @retval Distance in mm, 0 = no echo / out of range.
 */
uint16_t UltrasonicOvertake_GetDistance(void);

#endif /* __ULTRASONIC_OVERTAKE_H */
