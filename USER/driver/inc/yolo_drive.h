/*
 * yolo_drive.h — YOLOv2 vision-driven driving (7-sign version)
 *
 * Receives road sign commands from K210 via USART2 (k210_comm.c ISR):
 *   H  → horn (auto-off),  L/R → overtake,
 *   1/2 (PARK1/PARK2) → stop,  W → slow,  F → resume
 *
 * Call YoloDrive_Init() once, YoloDrive_Tick() from SysTick every 1 ms.
 * Requires K210Comm_Init() called beforehand for RXNE interrupt.
 */

#ifndef __YOLO_DRIVE_H
#define __YOLO_DRIVE_H

#include <stdint.h>
#include "yolo_drive_config.h"

/* ---- Initialization ---- */

void YoloDrive_Init(void);

/**
 * @brief  1 ms tick — call from SysTick.
 *         Drives Overtake state machine + processes K210 sign commands.
 */
void YoloDrive_Tick(void);

/* ---- Status queries ---- */

/**
 * @brief  Get current yolo drive state name (for display).
 * @retval "IDLE", "OVT1", "FWD", "OVT2", "STOP", "SLOW", "HORN"
 */
const char *YoloDrive_GetStateName(void);

/**
 * @brief  Get the overtake sub-state name when maneuver is active.
 * @retval Overtake state name if active, otherwise same as GetStateName.
 */
const char *YoloDrive_GetDetailStateName(void);

/**
 * @brief  Get last received sign payload string.
 * @retval "H","L","R","1","2","W","F", or "-" if none.
 */
const char *YoloDrive_GetLastSign(void);

/**
 * @brief  Check if yolo drive is executing a maneuver (not idle/stopped).
 */
uint8_t YoloDrive_IsActive(void);

/**
 * @brief  Check if overtake sequence is complete and car is idle/stopped/slow.
 */
uint8_t YoloDrive_IsComplete(void);

#endif /* __YOLO_DRIVE_H */
