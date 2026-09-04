/*
 * ultrasonic_overtake.h — Ultrasonic-triggered overtaking driver
 *
 * Thin wrapper: uses ultrasonic distance to decide when to trigger
 * the generic Overtake maneuver. All maneuver logic is in overtake.c.
 *
 * Call UltrasonicOvertake_Tick() from SysTick every 1 ms.
 */

#ifndef __ULTRASONIC_OVERTAKE_H
#define __ULTRASONIC_OVERTAKE_H

#include <stdint.h>
#include "ultrasonic_overtake_config.h"

/* ---- Initialization ---- */

/**
 * @brief  Initialize ultrasonic overtaking.
 *         Resets overtake state machine, starts driving forward.
 *         Does NOT call Ultrasonic_Init() — caller must do that.
 */
void UltrasonicOvertake_Init(void);

/**
 * @brief  1 ms tick — call from SysTick.
 *         Triggers ultrasonic, checks distance, delegates to Overtake driver.
 */
void UltrasonicOvertake_Tick(void);

/**
 * @brief  Get current state name string (for display).
 *         Returns overtake state name if maneuver active, "FWD" otherwise.
 */
const char *UltrasonicOvertake_GetStateName(void);

/**
 * @brief  Get current distance in mm (last ultrasonic reading).
 */
uint16_t UltrasonicOvertake_GetDistance(void);

/**
 * @brief  Get in-state timer value (ms) for debug display.
 */
uint16_t UltrasonicOvertake_GetTimer(void);

#endif /* __ULTRASONIC_OVERTAKE_H */
