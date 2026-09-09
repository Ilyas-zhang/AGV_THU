/*
 * overtake.h — Generic overtaking maneuver driver
 *
 * Supports left and right overtaking:
 *   Left overtake:  IDLE → STOP → ROT_LEFT → PASS → ROT_RIGHT → IDLE
 *   Right overtake: IDLE → STOP → ROT_RIGHT → PASS → ROT_LEFT  → IDLE
 *
 * Usage:
 *   1. Call Overtake_Init() once (loads default config from overtake_config.h)
 *   2. Optionally call Overtake_SetConfig() to override parameters
 *   3. Call Overtake_Tick() from SysTick every 1 ms
 *   4. When obstacle detected: call Overtake_Trigger(OVERTAKE_DIR_LEFT/RIGHT)
 *   5. Check Overtake_IsActive() / Overtake_IsComplete()
 */

#ifndef __OVERTAKE_H
#define __OVERTAKE_H

#include <stdint.h>

/* ---- Configuration struct ---- */

typedef struct {
    uint16_t stop_delay_ms;   /* 停车等待时间 ms */
    uint16_t rotate_ms;       /* 原地旋转时间 ms（调到≈90°） */
    uint16_t pass_ms;         /* 直行超越时间 ms */
    uint16_t rotate_speed;    /* 原地旋转速度 (0~3599, 必须 ≥ 2100) */
    uint16_t pass_speed;      /* 直行超越速度 (0~3599, 必须 ≥ 2100) */
} Overtake_Config;

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

/* ---- Configuration ---- */

/**
 * @brief  Override overtake parameters at runtime.
 *         Call before Overtake_Trigger() to use custom timings/speeds.
 *         Overtake_Init() resets to defaults from overtake_config.h.
 * @param  cfg  Pointer to config struct (copied internally).
 */
void Overtake_SetConfig(const Overtake_Config *cfg);

/**
 * @brief  Fill cfg with default values from overtake_config.h macros.
 * @param  cfg  Pointer to config struct to populate.
 */
void Overtake_GetDefaultConfig(Overtake_Config *cfg);

/* ---- Direction constants ---- */

#define OVERTAKE_DIR_LEFT        1       /* 左超车：先左旋后右旋 */
#define OVERTAKE_DIR_RIGHT      (-1)    /* 右超车：先右旋后左旋 */

/* ---- Status queries ---- */

uint8_t  Overtake_IsActive(void);
uint8_t  Overtake_IsComplete(void);

/**
 * @brief  Get current state name (for display).
 * @retval "IDLE", "STOP", "ROT1", "PASS", "ROT2"
 */
const char *Overtake_GetStateName(void);

uint16_t Overtake_GetTimer(void);

/**
 * @brief  Get overtake direction (for display).
 * @retval OVERTAKE_DIR_LEFT or OVERTAKE_DIR_RIGHT, 0 if IDLE
 */
int8_t Overtake_GetDirection(void);

#endif /* __OVERTAKE_H */
