/*
 * overtake.h — Generic overtaking maneuver driver
 *
 * Supports left and right overtaking:
 *   Left overtake:  IDLE → STOP → ROT_LEFT → PASS → ROT_RIGHT → IDLE
 *   Right overtake: IDLE → STOP → ROT_RIGHT → PASS → ROT_LEFT  → IDLE
 *
 * Usage:
 *   1. Call Overtake_Init() once
 *   2. Call Overtake_Tick() from SysTick every 1 ms
 *   3. When obstacle detected: call Overtake_Trigger(OVERTAKE_DIR_LEFT/RIGHT)
 *   4. Check Overtake_IsActive() / Overtake_IsComplete()
 */

#ifndef __OVERTAKE_H
#define __OVERTAKE_H

#include <stdint.h>
#include "overtake_config.h"

/* ---- Initialization ---- */

void Overtake_Init(void);

/**
 * @brief  1 ms tick — call from SysTick.
 */
void Overtake_Tick(void);

/**
 * @brief  Trigger overtaking maneuver.
 * @param  direction  OVERTAKE_DIR_LEFT or OVERTAKE_DIR_RIGHT
 *         Only works when state is IDLE.
 */
void Overtake_Trigger(int8_t direction);

/* ---- Status queries ---- */

uint8_t  Overtake_IsActive(void);
uint8_t  Overtake_IsComplete(void);

/**
 * @brief  Get current state name (for display).
 * @retval "IDLE", "STOP", "RL", "PASS", "RR"
 */
const char *Overtake_GetStateName(void);

uint16_t Overtake_GetTimer(void);

/**
 * @brief  Get overtake direction (for display).
 * @retval OVERTAKE_DIR_LEFT or OVERTAKE_DIR_RIGHT, 0 if IDLE
 */
int8_t Overtake_GetDirection(void);

#endif /* __OVERTAKE_H */
