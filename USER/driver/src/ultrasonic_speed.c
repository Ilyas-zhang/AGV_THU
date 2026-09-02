/*
 * ultrasonic_speed.c — Speed estimation from ultrasonic distance readings
 *
 * Uses DWT cycle counter for precise Δt between samples.
 * EMA filter: speed_f = (speed_raw + 3 * speed_f_prev) / 4
 *             (α = 0.25, smooths noise without much lag)
 */

#include "ultrasonic_speed.h"
#include "main.h"

/* ---- DWT micros (shared with ultrasonic.c, already initialized) ---- */
#define CPU_FREQ_MHZ  72

static inline uint32_t DWT_Micros(void)
{
    return DWT->CYCCNT / CPU_FREQ_MHZ;
}

/* ---- private state ---- */
static uint16_t prev_dist_mm   = 0;     /* previous distance sample */
static uint32_t prev_time_us   = 0;     /* DWT μs at previous sample */
static int16_t  speed_raw_cm_s = 0;     /* raw speed (cm/s), last update */
static int16_t  speed_ema_cm_s = 0;     /* EMA-filtered speed (cm/s) */
static uint8_t  first_sample   = 1;     /* skip delta on first sample */

/* ---- public API ---- */

void UltrasonicSpeed_Init(void)
{
    prev_dist_mm   = 0;
    prev_time_us   = DWT_Micros();
    speed_raw_cm_s = 0;
    speed_ema_cm_s = 0;
    first_sample   = 1;
}

void UltrasonicSpeed_Update(uint16_t distance_mm)
{
    uint32_t now_us = DWT_Micros();

    if (first_sample) {
        /* No delta on first sample — just store baseline */
        prev_dist_mm = distance_mm;
        prev_time_us = now_us;
        first_sample = 0;
        return;
    }

    uint32_t dt_us = now_us - prev_time_us;

    if (dt_us < 1000) {
        /* Too fast (< 1 ms) — ignore, likely noise */
        return;
    }

    /*
     * speed = -Δd / Δt  (negative delta = approaching = positive speed)
     *
     * speed_cm_per_s = -(distance_mm - prev_dist_mm) / 10  /  (dt_us / 1000000)
     *                = -(distance_mm - prev_dist_mm) * 100000  /  dt_us
     *
     * To avoid overflow with 32-bit math:
     *   delta_mm fits in int16_t (±several meters)
     *   delta_mm * 100000 fits in int32_t (±32767 * 100000 = ±3.2 billion, just fits)
     */
    int32_t delta_mm = (int32_t)distance_mm - (int32_t)prev_dist_mm;
    int32_t speed_cm_s = -(delta_mm * 100000) / (int32_t)dt_us;

    /* Clamp to int16_t range */
    if (speed_cm_s > 32767) speed_cm_s = 32767;
    if (speed_cm_s < -32768) speed_cm_s = -32768;

    speed_raw_cm_s = (int16_t)speed_cm_s;

    /* EMA filter: α = 0.25 → speed_ema = (raw + 3 * ema_prev) / 4 */
    speed_ema_cm_s = (speed_raw_cm_s + 3 * speed_ema_cm_s + 2) / 4;

    /* Store for next delta */
    prev_dist_mm = distance_mm;
    prev_time_us = now_us;
}

int16_t UltrasonicSpeed_GetCmPerS(void)
{
    return speed_ema_cm_s;
}

int16_t UltrasonicSpeed_GetRawCmPerS(void)
{
    return speed_raw_cm_s;
}
