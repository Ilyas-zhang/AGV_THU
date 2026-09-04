/*
 * ir_remote.h — NEC protocol IR remote control driver
 *
 * IR receiver on PG11 (active-low, idle HIGH, falling-edge triggered).
 * NEC protocol: 9 ms AGC + 4.5 ms gap + 32-bit data.
 *   Data = [addr(8) | ~addr(8) | cmd(8) | ~cmd(8)]
 *   Repeat code: 9 ms AGC + 2.25 ms gap
 *
 * Non-blocking: EXTI ISR only timestamps edges, decoding is done
 * via DWT µs delta between consecutive falling edges.
 * Call IRRemote_EXTI_Handler() from HAL_GPIO_EXTI_Callback.
 */

#ifndef __IR_REMOTE_H
#define __IR_REMOTE_H

#include <stdint.h>
#include "ir_remote_config.h"

/* ---- Receiver pin (not in CubeMX, defined manually) ---- */

#define IRREMOTE_GPIO_Port    GPIOG
#define IRREMOTE_Pin         GPIO_PIN_11

/* ---- NEC protocol timing (fixed by protocol, not tunable) ---- */

/*
 * Falling-edge deltas for NEC protocol (receiver output, active-low):
 *
 *   Header:   9 ms low + 4.5 ms high = 13,500 µs  (start of new frame)
 *   Repeat:   9 ms low + 2.25 ms high = 11,250 µs  (repeat code)
 *   Bit 0:    562.5 µs low + 562.5 µs high = 1,125 µs
 *   Bit 1:    562.5 µs low + 1,687.5 µs high = 2,250 µs
 *
 * Classification uses center-point ± tolerance.
 * Tolerances and frame timeout are in ir_remote_config.h.
 */

#define NEC_HEADER_US        13500   /* 9 ms + 4.5 ms */
#define NEC_REPEAT_US        11250   /* 9 ms + 2.25 ms */
#define NEC_BIT1_US           2250   /* 562.5 µs + 1,687.5 µs */
#define NEC_BIT0_US           1125   /* 562.5 µs + 562.5 µs */

/* Invalid / no-data sentinel */
#define IRREMOTE_NO_CODE     0xFF

/* ---- API ---- */

/**
 * @brief  Initialize IR remote driver.
 *         Configures PG11 as EXTI falling-edge with pull-up,
 *         enables NVIC (EXTI15_10_IRQn), initializes DWT.
 */
void IRRemote_Init(void);

/**
 * @brief  EXTI handler — call from HAL_GPIO_EXTI_Callback when
 *         GPIO_Pin == IRREMOTE_Pin.
 *         Timestamps the falling edge and runs the decode state machine.
 */
void IRRemote_EXTI_Handler(void);

/**
 * @brief  1 ms tick — call from SysTick.
 *         Handles frame timeout (reset if stuck mid-frame).
 */
void IRRemote_Tick(void);

/**
 * @brief  Check if a new IR code is available (consume-on-read).
 * @retval 1 = new code available, 0 = nothing new
 */
uint8_t IRRemote_HasNewCode(void);

/**
 * @brief  Get last valid command byte.
 * @retval Command byte (0x00–0xFE), or IRREMOTE_NO_CODE if none
 */
uint8_t IRRemote_GetCode(void);

/**
 * @brief  Get full 32-bit raw NEC frame (addr|~addr|cmd|~cmd).
 * @retval 32-bit data, MSB = first received bit
 */
uint32_t IRRemote_GetRaw(void);

/**
 * @brief  Check if last reception was a repeat code.
 * @retval 1 = repeat, 0 = new frame
 */
uint8_t IRRemote_IsRepeat(void);

#endif /* __IR_REMOTE_H */
