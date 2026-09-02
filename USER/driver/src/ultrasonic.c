/*
 * ultrasonic.c — HC-SR04 ultrasonic ranging driver
 *
 *   Trigger (PF11): send 10 μs pulse → module emits 8× 40kHz burst
 *   Echo   (PF12): goes HIGH, stays HIGH for round-trip time, then LOW
 *
 *   Distance (mm) = echo_time_μs × 340 / 2 / 1000
 *                 ≈ echo_time_μs × 5 / 29
 *
 *   Distance (cm) ≈ echo_time_μs / 58
 *
 * Timing: DWT cycle counter at 72 MHz → 1 μs = 72 cycles
 */

#include "ultrasonic.h"
#include "main.h"

/* ---- DWT helpers ---- */
#define CPU_FREQ_MHZ   72

static inline void DWT_Init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL  |= DWT_CTRL_CYCCNTENA_Msk;
}

static inline uint32_t DWT_Micros(void)
{
    return DWT->CYCCNT / CPU_FREQ_MHZ;
}

static inline void DWT_DelayUs(uint32_t us)
{
    uint32_t start = DWT_Micros();
    while ((DWT_Micros() - start) < us) ;
}

/* ---- private state ---- */
static volatile uint32_t echo_start   = 0;    /* DWT μs at rising edge */
static volatile uint16_t distance_mm  = 0;    /* last measured distance (mm) */
static volatile uint8_t  echo_active  = 0;    /* 1 = echo high, measuring */
static volatile uint8_t  data_ready   = 0;    /* 1 = new measurement available */

#define MAX_ECHO_US  35000   /* ~6 m limit, timeout */

/* ---- public API ---- */

void Ultrasonic_Init(void)
{
    DWT_Init();

    /* Reconfigure Echo pin (PF12) as EXTI rising+falling */
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin   = SonicEcho_Pin;
    GPIO_InitStruct.Mode  = GPIO_MODE_IT_RISING_FALLING;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    HAL_GPIO_Init(SonicEcho_GPIO_Port, &GPIO_InitStruct);

    /* Enable EXTI15_10 interrupt in NVIC */
    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

    /* Trigger idle LOW */
    HAL_GPIO_WritePin(SonicTrig_GPIO_Port, SonicTrig_Pin, GPIO_PIN_RESET);

    echo_active  = 0;
    distance_mm  = 0;
    data_ready   = 0;
}

void Ultrasonic_Trigger(void)
{
    /* Send 10 μs pulse on Trigger */
    HAL_GPIO_WritePin(SonicTrig_GPIO_Port, SonicTrig_Pin, GPIO_PIN_SET);
    DWT_DelayUs(10);
    HAL_GPIO_WritePin(SonicTrig_GPIO_Port, SonicTrig_Pin, GPIO_PIN_RESET);
}

uint16_t Ultrasonic_GetDistance(void)
{
    return distance_mm;
}

uint16_t Ultrasonic_GetDistanceCm(void)
{
    /* distance_mm / 10 = cm, with rounding */
    return (distance_mm + 5) / 10;
}

uint8_t Ultrasonic_IsReady(void)
{
    if (data_ready) {
        data_ready = 0;
        return 1;
    }
    return 0;
}

void Ultrasonic_EXTI_Handler(void)
{
    if (HAL_GPIO_ReadPin(SonicEcho_GPIO_Port, SonicEcho_Pin) == GPIO_PIN_SET) {
        /* ---- Rising edge: Echo went HIGH → start timing ---- */
        echo_start  = DWT_Micros();
        echo_active = 1;
    } else if (echo_active) {
        /* ---- Falling edge: Echo went LOW → compute distance ---- */
        uint32_t elapsed_us = DWT_Micros() - echo_start;
        echo_active = 0;

        if (elapsed_us < MAX_ECHO_US) {
            /* distance_mm = elapsed_us * 5 / 29 */
            distance_mm = (uint16_t)((elapsed_us * 5 + 14) / 29);
            data_ready  = 1;
        } else {
            distance_mm = 0;   /* out of range */
            data_ready  = 1;
        }
    }
}
