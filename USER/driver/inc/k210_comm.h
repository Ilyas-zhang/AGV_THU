/*
 * k210_comm.h — STM32 ↔ K210 串口通讯驱动 (USART2)
 *
 * 接收：裸字符模式，K210 直接发单字符命令 (L/R/H/W/F/1/2)
 *       RXNE 中断收到即置位，最低延时
 *
 * 发送：仍用 $payload# 帧格式（STM32→K210 心跳等）
 *
 * USART2 配置：
 *   Baud: 115200, 8N1
 *   重映射到 PD5(TX)/PD6(RX)
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
 *         例如 K210Comm_SendFrame("alive") 发送 "$alive#"
 */
void K210Comm_SendFrame(const char *payload);

/* ---- 接收 ---- */

/**
 * @brief  USART2 接收中断处理。
 *         从 USART2 读取 1 字节，直接存储为消息。
 *         需在 USART2_IRQHandler 中调用。
 */
void K210Comm_IRQHandler(void);

/**
 * @brief  查询是否有新的接收字符。
 * @retval 1 = 有新消息可读，0 = 无
 */
uint8_t K210Comm_HasMessage(void);

/**
 * @brief  获取最近接收的字符。
 * @retval 指向内部缓冲区的字符串（单字符 + null）。
 */
const char *K210Comm_GetMessage(void);

/**
 * @brief  清除"有新消息"标志。
 */
void K210Comm_ClearFlag(void);

/* ---- DEBUG ---- */
extern volatile uint16_t k210_rx_byte_cnt;

#endif /* __K210_COMM_H */
