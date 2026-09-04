/*
 * k210_comm.h — STM32 ↔ K210 串口通讯驱动 (USART2)
 *
 * 帧格式：$payload#  （与 K210 端一致）
 *   $ = 帧起始标记
 *   payload = 有效数据（不含 $ 和 #）
 *   # = 帧结束标记
 *
 * 接收：中断驱动，自动解析 $...# 帧
 * 发送：轮询发送（HAL_UART_Transmit 阻塞，适用于低频发送）
 *
 * USART2 配置（需在 CubeMX 中启用）：
 *   Baud: 115200, 8N1
 *   TX: PA2, RX: PA3（默认无重映射）
 *   全局中断: 使能
 */

#ifndef __K210_COMM_H
#define __K210_COMM_H

#include <stdint.h>
#include "k210_comm_config.h"

/* ---- 初始化 ---- */

/**
 * @brief  初始化 K210 通讯驱动。
 *         使能 USART2 接收中断，清空接收缓冲。
 *         需在 MX_USART2_UART_Init() 之后调用。
 */
void K210Comm_Init(void);

/* ---- 发送 ---- */

/**
 * @brief  发送单个字节（阻塞等待 TXE）。
 */
void K210Comm_SendByte(uint8_t data);

/**
 * @brief  发送 null 结尾的字符串（阻塞）。
 */
void K210Comm_SendString(const char *str);

/**
 * @brief  以 $...# 帧格式发送 payload（阻塞）。
 *         例如 K210Comm_SendFrame("hello") 发送 "$hello#"
 */
void K210Comm_SendFrame(const char *payload);

/* ---- 接收 ---- */

/**
 * @brief  USART2 接收中断处理。
 *         从 USART2 读取 1 字节，送入帧解析器。
 *         需在 USART2_IRQHandler 中调用。
 */
void K210Comm_IRQHandler(void);

/**
 * @brief  查询是否有完整的接收帧。
 * @retval 1 = 有新消息可读，0 = 无
 *         调用后不清除标志（需手动调 K210Comm_ClearFlag）。
 */
uint8_t K210Comm_HasMessage(void);

/**
 * @brief  获取最近接收帧的 payload（不含 $ #）。
 * @retval 指向内部缓冲区的字符串，null 结尾。
 *         下次收到新帧后会被覆盖。
 */
const char *K210Comm_GetMessage(void);

/**
 * @brief  清除"有新消息"标志。
 */
void K210Comm_ClearFlag(void);

#endif /* __K210_COMM_H */
