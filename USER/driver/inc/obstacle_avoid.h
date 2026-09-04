/*
 * obstacle_avoid.h — Ultrasonic obstacle avoidance driver
 *
 * State-machine driven avoidance behavior:
 *   FORWARD → SLOW → STOP → BACKUP → TURN → FORWARD
 *
 * Uses the existing non-blocking Ultrasonic_* API (DWT + EXTI).
 * Call ObstacleAvoid_Tick() from SysTick every 1 ms.
 */

#ifndef __OBSTACLE_AVOID_H
#define __OBSTACLE_AVOID_H

#include <stdint.h>
#include "obstacle_avoid_config.h"

/* ===== API ===== */

/**
 * @brief  Initialize obstacle avoidance state machine.
 *         Resets state to FORWARD, clears timers.
 *         Does NOT call Ultrasonic_Init() — caller must do that.
 */
void ObstacleAvoid_Init(void);

/**
 * @brief  1 ms tick handler — call from SysTick.
 *         Periodically triggers ultrasonic measurement and
 *         drives the avoidance state machine.
 */
void ObstacleAvoid_Tick(void);

/**
 * @brief  Set forward and slow speeds at runtime.
 * @param  forward  Forward speed (0~3599)
 * @param  slow     Slow/approach speed (0~3599)
 */
void ObstacleAvoid_SetSpeed(int16_t forward, int16_t slow);

/**
 * @brief  Get current state name string (for display).
 * @retval Short state name: "FWD", "SLOW", "STOP", "BACK", "TURN"
 */
const char *ObstacleAvoid_GetStateName(void);

/**
 * @brief  Get current distance in mm (last ultrasonic reading).
 * @retval Distance in mm, 0 = no echo / out of range.
 */
uint16_t ObstacleAvoid_GetDistance(void);

#endif /* __OBSTACLE_AVOID_H */
