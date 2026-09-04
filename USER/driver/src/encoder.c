/*
 * encoder.c — 四路正交编码器驱动
 *
 *   采用中点计数法：初始化时将 CNT 设为 0x7FFF (32767)，
 *   每次读取后重置 CNT = 0x7FFF，增量 = 0x7FFF - (int16_t)CNT。
 *   正值 = 正转，负值 = 反转。
 *
 *   Timer → Encoder 映射（与硬件接线对应，同参考例程）：
 *     Encoder1 → TIM4 (PD12/PD13)
 *     Encoder2 → TIM2 (PA15/PB3)
 *     Encoder3 → TIM5 (PA0/PA1)
 *     Encoder4 → TIM3 (PA6/PA7)
 *
 *   Encoder_Tick() 需在 SysTick 1 kHz 驱动。
 */

#include "encoder.h"
#include "encoder_config.h"
#include "main.h"
#include "tim.h"

/* ---- 常量 ---- */
#define ENC_MIDPOINT    0x7FFF   /* CNT 中点，用于有符号增量读取 */

/* ---- private state ---- */
static int16_t enc_delta[4];     /* 本次 Tick 增量 (Encoder1~4) */
static int32_t enc_total[4];     /* 累计总计 */
static int16_t enc_speed[4];     /* 速度 (counts / Tick 间隔) */

/* ---- private: 读取单路增量并重置 CNT ---- */
static int16_t encoder_read_raw(uint8_t id)
{
    int16_t cnt_val = 0;
    switch (id) {
    case 1:  /* Encoder1 → TIM4 */
        cnt_val = (int16_t)TIM4->CNT;
        TIM4->CNT = ENC_MIDPOINT;
        break;
    case 2:  /* Encoder2 → TIM2 */
        cnt_val = (int16_t)TIM2->CNT;
        TIM2->CNT = ENC_MIDPOINT;
        break;
    case 3:  /* Encoder3 → TIM5 */
        cnt_val = (int16_t)TIM5->CNT;
        TIM5->CNT = ENC_MIDPOINT;
        break;
    case 4:  /* Encoder4 → TIM3 */
        cnt_val = (int16_t)TIM3->CNT;
        TIM3->CNT = ENC_MIDPOINT;
        break;
    default:
        return 0;
    }
    return ENC_MIDPOINT - cnt_val;   /* 正值=正转，负值=反转 */
}

/* ---- private: 应用方向反转 ---- */
static int16_t encoder_apply_reverse(uint8_t id, int16_t raw)
{
    switch (id) {
    case 1: return ENCODER1_REVERSE ? -raw : raw;
    case 2: return ENCODER2_REVERSE ? -raw : raw;
    case 3: return ENCODER3_REVERSE ? -raw : raw;
    case 4: return ENCODER4_REVERSE ? -raw : raw;
    default: return raw;
    }
}

/* ========== 初始化 ========== */

void Encoder_Init(void)
{
    /* 启动所有编码器定时器 */
    HAL_TIM_Encoder_Start(&htim4, TIM_CHANNEL_ALL);   /* Encoder1 */
    HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_ALL);   /* Encoder2 */
    HAL_TIM_Encoder_Start(&htim5, TIM_CHANNEL_ALL);   /* Encoder3 */
    HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL);   /* Encoder4 */

    /* CNT 置中点，使增量读取为有符号值 */
    TIM4->CNT = ENC_MIDPOINT;
    TIM2->CNT = ENC_MIDPOINT;
    TIM5->CNT = ENC_MIDPOINT;
    TIM3->CNT = ENC_MIDPOINT;

    /* 清零状态 */
    Encoder_Reset();
}

/* ========== 周期采样 ========== */

void Encoder_Tick(void)
{
    for (uint8_t i = 0; i < 4; i++) {
        uint8_t id = i + 1;   /* 编码器编号 1~4 */
        int16_t raw = encoder_read_raw(id);
        int16_t d   = encoder_apply_reverse(id, raw);

        enc_delta[i] = d;
        enc_total[i] += d;
        enc_speed[i] = d;     /* 1 ms Tick 时 speed == delta */
    }
}

/* ========== 读取接口 ========== */

int16_t Encoder_GetDelta(uint8_t id)
{
    if (id < 1 || id > 4) return 0;
    return enc_delta[id - 1];
}

int32_t Encoder_GetTotal(uint8_t id)
{
    if (id < 1 || id > 4) return 0;
    return enc_total[id - 1];
}

int16_t Encoder_GetSpeed(uint8_t id)
{
    if (id < 1 || id > 4) return 0;
    return enc_speed[id - 1];
}

void Encoder_GetAllDeltas(int16_t *deltas)
{
    for (uint8_t i = 0; i < 4; i++) {
        deltas[i] = enc_delta[i];
    }
}

void Encoder_Reset(void)
{
    for (uint8_t i = 0; i < 4; i++) {
        enc_delta[i] = 0;
        enc_total[i] = 0;
        enc_speed[i] = 0;
    }
}
