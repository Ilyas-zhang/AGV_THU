/*
 * ir_avoid_drive.h — IR obstacle avoidance driving driver
 *
 * Uses the generic Overtake driver for avoidance maneuvers:
 *   Left obstacle  → right overtake
 *   Right obstacle → left overtake
 *   Both obstacles → left overtake
 *   No obstacle    → forward
 *
 * IRAvoid_Tick() must also run from SysTick for debounce filtering.
 */

#ifndef __IR_AVOID_DRIVE_H
#define __IR_AVOID_DRIVE_H

#include <stdint.h>
#include "ir_avoid_drive_config.h"

void IRAvoidDrive_Init(void);
void IRAvoidDrive_Tick(void);

/**
 * @brief  Get current state name (for display).
 * @retval "FWD" or overtake state name
 */
const char *IRAvoidDrive_GetStateName(void);

#endif /* __IR_AVOID_DRIVE_H */
