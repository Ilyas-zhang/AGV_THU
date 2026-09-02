/*
 * ir_remote.c — NEC protocol IR remote control driver
 *
 * IR receiver on PG11 (active-low, idle HIGH, falling-edge triggered).
 *
 * Non-blocking decode: EXTI ISR timestamps each falling edge with DWT,
 * then classifies the delta from the previous falling edge:
 *
 *   ~13,500 µs → NEC header (9ms AGC + 4.5ms gap) → start new frame
 *   ~11,250 µs → NEC repeat (9ms AGC + 2.25ms gap) → repeat flag
 *   ~ 2,250 µs → bit 1 (562.5µs mark + 1,687.5µs space)
 *   ~ 1,125 µs → bit 0 (562.5µs mark +   562.5µs space)
 *
 * After 32 bits, validates NEC checksum: addr==~addr_inv && cmd==~cmd_inv.
 * ISR work is ~1 µs per edge — no blocking busy-wait.
 */

#include "ir_remote.h"
#include "main.h"

/* ---- DWT µs timing (idempotent, shared-safe) ---- */

#define CPU_FREQ_MHZ   72

static inline void DWT_Init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    /* Only reset CYCCNT if not already running — avoids disrupting ultrasonic */
    if (!(DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk)) {
        DWT->CYCCNT = 0;
        DWT->CTRL  |= DWT_CTRL_CYCCNTENA_Msk;
    }
}

static inline uint32_t DWT_Micros(void)
{
    return DWT->CYCCNT / CPU_FREQ_MHZ;
}

/* ---- Decode state machine ---- */

typedef enum {
    IR_IDLE,       /* Waiting for header */
    IR_RECEIVING   /* Collecting 32 data bits */
} IR_State;

static volatile IR_State   state        = IR_IDLE;
static volatile uint32_t   last_edge_us = 0;    /* DWT µs at last falling edge */
static volatile uint8_t    bit_count    = 0;     /* Bits received (0–32) */
static volatile uint32_t   frame_data   = 0;    /* 32-bit shift register */

static volatile uint8_t    last_cmd     = IRREMOTE_NO_CODE;
static volatile uint32_t   last_raw     = 0;
static volatile uint8_t    data_ready   = 0;
static volatile uint8_t    is_repeat    = 0;

/* ---- Classification helper ---- */

/* Classify a falling-edge delta into one of 4 categories.
 * Returns: 0=bit0, 1=bit1, 2=header, 3=repeat, 0xFF=error */
static uint8_t classify_delta(uint32_t delta_us)
{
    /* Header: 13,500 ± 1,500 µs */
    if (delta_us >= NEC_HEADER_US - NEC_HEADER_TOL &&
        delta_us <= NEC_HEADER_US + NEC_HEADER_TOL) {
        return 2;  /* header */
    }

    /* Repeat: 11,250 ± 1,000 µs */
    if (delta_us >= NEC_REPEAT_US - NEC_REPEAT_TOL &&
        delta_us <= NEC_REPEAT_US + NEC_REPEAT_TOL) {
        return 3;  /* repeat */
    }

    /* Bit 1: 2,250 ± 400 µs */
    if (delta_us >= NEC_BIT1_US - NEC_BIT_TOL &&
        delta_us <= NEC_BIT1_US + NEC_BIT_TOL) {
        return 1;  /* bit 1 */
    }

    /* Bit 0: 1,125 ± 400 µs */
    if (delta_us >= NEC_BIT0_US - NEC_BIT_TOL &&
        delta_us <= NEC_BIT0_US + NEC_BIT_TOL) {
        return 0;  /* bit 0 */
    }

    return 0xFF;   /* unrecognized */
}

/* ---- Public API ---- */

void IRRemote_Init(void)
{
    /* DWT cycle counter */
    DWT_Init();

    /* Configure PG11 as EXTI falling-edge, pull-up */
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin   = IRREMOTE_Pin;
    GPIO_InitStruct.Mode  = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull  = GPIO_PULLUP;
    HAL_GPIO_Init(IRREMOTE_GPIO_Port, &GPIO_InitStruct);

    /* NVIC — share EXTI15_10 with ultrasonic (PF12).
     * Priority 0,0 already set by Ultrasonic_Init(); setting again is harmless. */
    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

    /* Reset state */
    state        = IR_IDLE;
    last_edge_us = 0;
    bit_count    = 0;
    frame_data   = 0;
    last_cmd     = IRREMOTE_NO_CODE;
    last_raw     = 0;
    data_ready   = 0;
    is_repeat    = 0;
}

void IRRemote_EXTI_Handler(void)
{
    uint32_t now = DWT_Micros();
    uint32_t delta = now - last_edge_us;
    last_edge_us = now;

    uint8_t cls = classify_delta(delta);

    switch (cls) {
    case 2:  /* Header — start of new frame */
        state      = IR_RECEIVING;
        bit_count  = 0;
        frame_data = 0;
        is_repeat  = 0;
        break;

    case 3:  /* Repeat code */
        if (last_cmd != IRREMOTE_NO_CODE) {
            is_repeat  = 1;
            data_ready = 1;
        }
        state     = IR_IDLE;
        bit_count = 0;
        break;

    case 0:  /* Bit 0 */
    case 1:  /* Bit 1 */
        if (state == IR_RECEIVING) {
            frame_data <<= 1;
            frame_data  |= cls;   /* cls is 0 or 1 */
            if (++bit_count >= 32) {
                /* Full frame received — validate NEC checksum */
                uint8_t addr     = (uint8_t)(frame_data >> 24);
                uint8_t addr_inv = (uint8_t)(frame_data >> 16);
                uint8_t cmd      = (uint8_t)(frame_data >> 8);
                uint8_t cmd_inv  = (uint8_t)(frame_data);

                if ((addr == (uint8_t)~addr_inv) &&
                    (cmd == (uint8_t)~cmd_inv)) {
                    last_cmd   = cmd;
                    last_raw   = frame_data;
                    is_repeat  = 0;
                    data_ready = 1;
                }
                /* Checksum failed — discard, stay idle */
                state     = IR_IDLE;
                bit_count = 0;
            }
        }
        break;

    default:  /* Unrecognized delta — reset */
        state     = IR_IDLE;
        bit_count = 0;
        break;
    }
}

void IRRemote_Tick(void)
{
    /* Frame timeout: if receiving and no edge for >15 ms, reset */
    if (state == IR_RECEIVING) {
        uint32_t elapsed = DWT_Micros() - last_edge_us;
        if (elapsed > (uint32_t)NEC_FRAME_TIMEOUT_MS * 1000u) {
            state     = IR_IDLE;
            bit_count = 0;
        }
    }
}

uint8_t IRRemote_HasNewCode(void)
{
    if (data_ready) {
        data_ready = 0;
        return 1;
    }
    return 0;
}

uint8_t IRRemote_GetCode(void)
{
    return last_cmd;
}

uint32_t IRRemote_GetRaw(void)
{
    return last_raw;
}

uint8_t IRRemote_IsRepeat(void)
{
    return is_repeat;
}
