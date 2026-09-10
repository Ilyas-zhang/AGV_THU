#ifndef __IR_REMOTE_CONFIG_H
#define __IR_REMOTE_CONFIG_H

/*
 * 红外遥控 NEC 协议参数配置
 *
 * NEC 时序中心值由协议固定，无需调整
 * 可调的是容差带（环境噪声大时可放宽）和帧超时
 */

/* ---- 容差带 (μs) ---- */
#define NEC_HEADER_TOL           1500    /* 头码 ±1.5ms（中心 13.5ms） */
#define NEC_REPEAT_TOL           1000    /* 重复码 ±1.0ms（中心 11.25ms） */
#define NEC_BIT_TOL              400     /* 数据位 ±0.4ms */

/* ---- 帧超时 ---- */
#define NEC_FRAME_TIMEOUT_MS     15      /* 接收中途无沿超过此时间则复位 */

/* ---- 帧位数 ---- */
#define NEC_FRAME_BITS           32      /* NEC 标准帧: 8+8+8+8 */

#endif /* __IR_REMOTE_CONFIG_H */
