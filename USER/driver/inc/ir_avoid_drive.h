/*
 * ir_avoid_drive.h — IR obstacle avoidance driving driver
 *
 * Reactive state-machine driven avoidance:
 *   FORWARD → TURN_RIGHT / TURN_LEFT / BACKUP → ROTATE → FORWARD
 *
 * Uses the existing IRAvoid_* API with asymmetric debounce filter.
 * Call IRAvoidDrive_Tick() from SysTick every 1 ms.
 * IRAvoid_Tick() must also be enabled in SysTick for the filter to run.
 */

#ifndef __IR_AVOID_DRIVE_H
#define __IR_AVOID_DRIVE_H

#include <stdint.h>
#include "ir_avoid_drive_config.h"

/* ---- Initialization ---- */

/**
 * @brief  Initialize IR avoidance driving state machine.
 *         Resets state to FORWARD, clears timers.
 *         Does NOT call IRAvoid_Init() — caller must do that.
 */
void IRAvoidDrive_Init(void);

/**
 * @brief  1 ms tick handler — call from SysTick.
 *         Reads IR sensors and drives the avoidance state machine.
 *         IRAvoid_Tick() must also run from SysTick for debounce filtering.
 */
void IRAvoidDrive_Tick(void);

/**
 * @brief  Get current state name string (for display).
 * @retval Short state name: "FWD", "TR", "TL", "BACK", "ROT"
 */
const char *IRAvoidDrive_GetStateName(void);

#endif /* __IR_AVOID_DRIVE_H */
