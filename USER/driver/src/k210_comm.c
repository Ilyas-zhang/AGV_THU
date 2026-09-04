/*
 * k210_comm.c — STM32 ↔ K210 串口通讯驱动 (USART2)
 *
 *   帧协议：$payload#
 *     K210 发送 "$hello#" → STM32 收到后 payload = "hello"
 *     STM32 发送 "$world#" → K210 收到后解析同理
 *
 *   接收：USART2 RX 中断驱动，逐字节送入帧解析器
 *     - 收到 '$'：开始新帧，清空缓冲
 *     - 收到 '#'：帧结束，置位标志
 *     - 其他字符：存入缓冲（溢出则丢弃）
 *
 *   发送：轮询阻塞（HAL_UART_Transmit），适用于低频场景
 */

#include "k210_comm.h"
#include "main.h"
#include "usart.h"

/* ---- private state ---- */

static char    rx_buf[K210_RX_BUF_SIZE + 1];  /* +1 for null terminator */
static uint8_t rx_index   = 0;     /* 当前写入位置 */
static uint8_t rx_flag    = 0;     /* 1 = 帧起始 '$' 已收到，正在接收 */
static volatile uint8_t msg_ready = 0;   /* 1 = 完整帧已就绪 */

/* ========== 初始化 ========== */

void K210Comm_Init(void)
{
    /* 使能 USART2 接收中断（直接使能 RXNE，不使用 HAL_UART_Receive_IT） */
    __HAL_UART_ENABLE_IT(&huart2, UART_IT_RXNE);

    /* 清空状态 */
    rx_index   = 0;
    rx_flag    = 0;
    msg_ready  = 0;
    rx_buf[0]  = '\0';
}

/* ========== 发送 ========== */

void K210Comm_SendByte(uint8_t data)
{
    HAL_UART_Transmit(&huart2, &data, 1, K210_TX_BYTE_TIMEOUT);   /* timeout from config */
}

void K210Comm_SendString(const char *str)
{
    uint16_t len = 0;
    while (str[len] != '\0') len++;
    if (len > 0) {
        HAL_UART_Transmit(&huart2, (const uint8_t *)str, len, K210_TX_STRING_TIMEOUT);
    }
}

void K210Comm_SendFrame(const char *payload)
{
    K210Comm_SendByte('$');
    K210Comm_SendString(payload);
    K210Comm_SendByte('#');
}

/* ========== 接收：帧解析器 ========== */

void K210Comm_IRQHandler(void)
{
    uint8_t ch;

    /* 读取接收到的字节 */
    if (__HAL_UART_GET_FLAG(&huart2, UART_FLAG_RXNE) == RESET) return;
    ch = (uint8_t)(huart2.Instance->DR & 0xFF);

    if (ch == '$') {
        /* 帧起始：清空缓冲，开始接收 */
        rx_index = 0;
        rx_flag  = 1;
        rx_buf[0] = '\0';
    } else if (rx_flag && ch == '#') {
        /* 帧结束：null 终止，置位就绪 */
        rx_buf[rx_index] = '\0';
        rx_flag    = 0;
        msg_ready  = 1;
    } else if (rx_flag) {
        /* 帧内数据：存入缓冲 */
        if (rx_index < K210_RX_BUF_SIZE) {
            rx_buf[rx_index++] = (char)ch;
        } else {
            /* 缓冲溢出：丢弃当前帧 */
            rx_flag  = 0;
            rx_index = 0;
        }
    }
    /* 非帧内字节（rx_flag=0 且不是 '$'）：忽略 */
}

/* ========== 接收：查询接口 ========== */

uint8_t K210Comm_HasMessage(void)
{
    return msg_ready;
}

const char *K210Comm_GetMessage(void)
{
    return rx_buf;
}

void K210Comm_ClearFlag(void)
{
    msg_ready = 0;
}
