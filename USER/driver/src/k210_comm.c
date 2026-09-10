/*
 * k210_comm.c — STM32 ↔ K210 串口通讯驱动 (USART2)
 *
 *   裸字符模式：K210 直接发送单字符命令 (L/R/H/W/F/1/2)
 *   STM32 RXNE 中断收到字符即置位，无需帧协议解析
 *
 *   发送：仍保留 $payload# 帧格式（STM32→K210 心跳等）
 *   接收：裸字符，收到即生效，最低延时
 */

#include "k210_comm.h"
#include "main.h"
#include "usart.h"

/* ---- private state ---- */

static char    rx_buf[2];                /* 单字符 + null terminator */
static volatile uint8_t msg_ready = 0;  /* 1 = 新字符已就绪 */
volatile uint16_t k210_rx_byte_cnt = 0;  /* DEBUG: 收到的总字节数 */

/* ========== 初始化 ========== */

void K210Comm_Init(void)
{
    /* 使能 USART2 接收中断（直接使能 RXNE，不使用 HAL_UART_Receive_IT） */
    __HAL_UART_ENABLE_IT(&huart2, UART_IT_RXNE);

    /* 清空状态 */
    msg_ready  = 0;
    rx_buf[0]  = '\0';
    rx_buf[1]  = '\0';
}

/* ========== 发送 ========== */

void K210Comm_SendByte(uint8_t data)
{
    HAL_UART_Transmit(&huart2, &data, 1, K210_TX_BYTE_TIMEOUT);
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

/* ========== 接收：裸字符 ========== */

void K210Comm_IRQHandler(void)
{
    uint8_t ch;

    /* 读取接收到的字节 */
    if (__HAL_UART_GET_FLAG(&huart2, UART_FLAG_RXNE) == RESET) return;
    ch = (uint8_t)(huart2.Instance->DR & 0xFF);
    k210_rx_byte_cnt++;

    /* 直接存储为单字符消息，忽略 '$' '#' 等帧协议字符 */
    if (ch != '$' && ch != '#') {
        rx_buf[0] = (char)ch;
        rx_buf[1] = '\0';
        msg_ready = 1;
    }
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
