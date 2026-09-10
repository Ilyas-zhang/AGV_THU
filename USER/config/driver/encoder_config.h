#ifndef __ENCODER_CONFIG_H
#define __ENCODER_CONFIG_H

/*
 * 编码器参数配置
 *
 * ENCODER_CIRCLE = 减速比 × PPR × 倍频系数
 *   = 电机输出轴转一圈时，编码器产生的总计数。
 *
 * 使用方法：
 *   RPM = Encoder_GetSpeed(id) * 60000 / ENCODER_CIRCLE
 *
 * 调试方法：
 *   1. 手动转动车轮一圈，观察 Encoder_GetTotal() 的变化量
 *   2. 变化量应接近 ENCODER_CIRCLE，若不符则调整 PPR 或 MOTOR_REDUCTION
 *   3. 若某路正转时计数为负，将对应 ENCODERx_REVERSE 设为 1
 */

/* ---- 编码器线数 ---- */
#define ENCODER_PPR             13      /* 编码器每转脉冲数 (PPR) */
#define ENCODER_MODE_X          2       /* 倍频系数: TI1 mode = x2, TI12 mode = x4 */
#define MOTOR_REDUCTION         30      /* 电机减速比 (减速器输出轴:电机轴) */

/* 每转总计数 = 减速比 × PPR × 倍频 = 30 × 13 × 2 = 780 */
#define ENCODER_CIRCLE          (MOTOR_REDUCTION * ENCODER_PPR * ENCODER_MODE_X)

/* ---- 方向反转标志（1 = 计数取反，0 = 正常） ---- */
/* 如果某编码器正转时 delta 为负，将对应 REVERSE 设为 1 即可反转 */
#define ENCODER1_REVERSE        1       /* Encoder1 (TIM4, PD12/PD13) */
#define ENCODER2_REVERSE        1       /* Encoder2 (TIM2, PA15/PB3) */
#define ENCODER3_REVERSE        0       /* Encoder3 (TIM5, PA0/PA1) */
#define ENCODER4_REVERSE        0       /* Encoder4 (TIM3, PB4/PB5 partial remap) */

/* ---- RPM 换算 ---- */
/* RPM = speed_counts_per_ms × 60000 / ENCODER_CIRCLE */
#define ENCODER_RPM_SCALE       60000   /* ms → min 换算系数 */

#endif /* __ENCODER_CONFIG_H */
