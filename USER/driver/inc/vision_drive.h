/*
 * vision_drive.h — Vision-driven overtaking driver
 *
 * Receives road sign commands from K210 via USART2 (k210_comm.c ISR),
 * executes overtaking maneuvers:
 *   LEFT  sign → left overtake  → forward 1s → right overtake → resume
 *   RIGHT sign → right overtake → forward 1s → left overtake  → resume
 *   STOP  sign → stop
 *
 * All maneuver logic delegates to overtake.c;
 * timing/speed in overtake_config.h, vision params in vision_drive_config.h.
 *
 * Call VisionDrive_Init() once, VisionDrive_Tick() from SysTick every 1 ms.
 * Requires K210Comm_Init() called beforehand for RXNE interrupt.
 */

#ifndef __VISION_DRIVE_H
#define __VISION_DRIVE_H

#include <stdint.h>
#include "vision_drive_config.h"

/* ---- Initialization ---- */

void VisionDrive_Init(void);

/**
 * @brief  1 ms tick — call from SysTick.
 *         Drives Overtake state machine + processes K210 sign commands.
 */
void VisionDrive_Tick(void);

/* ---- Status queries ---- */

/**
 * @brief  Get current vision drive state name (for display).
 * @retval "IDLE", "OVT1", "FWD", "OVT2", "STOP"
 */
const char *VisionDrive_GetStateName(void);

/**
 * @brief  Get the overtake sub-state name when maneuver is active.
 * @retval Overtake state name if active, otherwise same as GetStateName.
 */
const char *VisionDrive_GetDetailStateName(void);

/**
 * @brief  Get last received sign character.
 * @retval 'R', 'L', 'S', 'a', or '-' if none.
 */
char VisionDrive_GetLastSign(void);

/**
 * @brief  Check if vision drive is executing a maneuver (not idle/stopped).
 */
uint8_t VisionDrive_IsActive(void);

#endif /* __VISION_DRIVE_H */
