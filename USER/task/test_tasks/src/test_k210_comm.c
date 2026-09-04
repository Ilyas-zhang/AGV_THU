/*
 * test_k210_comm.c — K210 路牌识别测试
 *
 * K210 (main.py) 发送路牌指令：
 *   "$R#" → RIGHT   "$L#" → LEFT   "$S#" → STOP
 *   "$H#" → HORN    "$2#" → ROOM2  "$1#" → ROOM1
 *
 * STM32 接收后在 OLED 显示路牌结果：
 *   Line 0: "Sign: RIGHT"
 *   Line 1: "RX: alive"
 *
 * USART2: PD5(TX)/PD6(RX) 115200 8N1（CubeMX 已配置重映射）
 * LED 闪烁 1 Hz 表示板子活着
 * OLED 128×32, Font_7x10
 */

#include "test_k210_comm.h"
#include "k210_comm.h"
#include "oled.h"
#include "led.h"
#include "led_config.h"
#include "main.h"
#include "usart.h"

#define DISPLAY_INTERVAL_MS    200     /* OLED 刷新间隔 */
#define LED_BLINK_MS           500     /* LED 闪烁半周期 */

static uint16_t disp_cnt     = 0;
static uint16_t led_cnt      = 0;
static char     last_sign    = '-';    /* 最近收到的路牌指令 */
static char     rx_buf[17]   = "-";    /* 显示用接收缓冲 */

/* 轮询接收局部状态 */
static char     rx_buf_local[17] = "";
static uint8_t  rx_index    = 0;
static uint8_t  rx_flag     = 0;
static uint8_t  msg_ready_local = 0;

/* 路牌指令转文字 */
static const char *sign_name(char c)
{
    switch (c) {
    case 'R': return "RIGHT ";
    case 'L': return "LEFT  ";
    case 'S': return "STOP  ";
    case 'H': return "HORN  ";
    case '2': return "ROOM2 ";
    case '1': return "ROOM1 ";
    case 'a': return "ALIVE ";
    default:  return "---   ";
    }
}

void TestK210Comm_Init(void)
{
    disp_cnt  = 0;
    led_cnt   = 0;
    last_sign = '-';
    rx_buf[0] = '-'; rx_buf[1] = '\0';

    /* 禁用 RXNE 中断，纯轮询接收 */
    __HAL_UART_DISABLE_IT(&huart2, UART_IT_RXNE);

    /* LED 亮一下表示重启完成 */
    LED_Set(LED_PRESET_STOP);

    OLED_Clear();
    OLED_GotoXY(0, 0);
    OLED_Puts("Sign: ---", &Font_7x10, OLED_COLOR_WHITE);
    OLED_GotoXY(0, 10);
    OLED_Puts("K210 Sign Test", &Font_7x10, OLED_COLOR_WHITE);
    OLED_Update();
}

void TestK210Comm_Tick(void)
{
    /* ---- 轮询 USART2 RX ---- */
    {
        uint32_t sr = USART2->SR;

        /* 清 ORE（读 DR 清除） */
        if (sr & USART_SR_ORE) {
            (void)USART2->DR;
        }
        /* RXNE → 读取一字节并送入帧解析器 */
        else if (sr & USART_SR_RXNE) {
            uint8_t ch = (uint8_t)(USART2->DR & 0xFF);
            k210_rx_byte_cnt++;

            if (ch == '$') {
                rx_index = 0;
                rx_flag = 1;
                rx_buf_local[0] = '\0';
            } else if (rx_flag && ch == '#') {
                rx_buf_local[rx_index] = '\0';
                rx_flag = 0;
                msg_ready_local = 1;
            } else if (rx_flag) {
                if (rx_index < 16) {
                    rx_buf_local[rx_index++] = (char)ch;
                } else {
                    rx_flag = 0;
                    rx_index = 0;
                }
            }
        }
    }

    /* 检查是否收到完整帧 */
    if (msg_ready_local) {
        msg_ready_local = 0;
        if (rx_buf_local[0] == 'R' || rx_buf_local[0] == 'L' || rx_buf_local[0] == 'S'
            || rx_buf_local[0] == 'H' || rx_buf_local[0] == '1' || rx_buf_local[0] == '2'
            || rx_buf_local[0] == 'a') {
            last_sign = rx_buf_local[0];
        }
        uint8_t i;
        for (i = 0; i < 16 && rx_buf_local[i] != '\0'; i++) {
            rx_buf[i] = rx_buf_local[i];
        }
        rx_buf[i] = '\0';
    }

    /* ---- LED 1 Hz 闪烁 ---- */
    if (++led_cnt >= LED_BLINK_MS) {
        led_cnt = 0;
        static uint8_t led_toggle = 0;
        led_toggle = !led_toggle;
        if (led_toggle) {
            LED_Set(100, 0, 0, BLINK_OFF, 100, 0, 0, BLINK_OFF);
        } else {
            LED_Set(0, 0, 0, BLINK_OFF, 0, 0, 0, BLINK_OFF);
        }
        /* 从 USART2 TX (PD5) 发送心跳 */
        K210Comm_SendFrame("alive");
    }

    /* ---- OLED 显示 ---- */
    if (++disp_cnt >= DISPLAY_INTERVAL_MS) {
        disp_cnt = 0;

        OLED_Clear();

        /* Line 0: "Sign: RIGHT " */
        OLED_GotoXY(0, 0);
        OLED_Puts("Sign:", &Font_7x10, OLED_COLOR_WHITE);
        OLED_Puts(sign_name(last_sign), &Font_7x10, OLED_COLOR_WHITE);

        /* Line 1: "RX: alive" */
        OLED_GotoXY(0, 10);
        OLED_Puts("RX: ", &Font_7x10, OLED_COLOR_WHITE);
        OLED_Puts(rx_buf, &Font_7x10, OLED_COLOR_WHITE);

        OLED_Update();
    }
}
