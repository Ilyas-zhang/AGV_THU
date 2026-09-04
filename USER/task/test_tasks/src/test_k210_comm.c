/*
 * test_k210_comm.c — K210 通讯测试
 *
 * 功能：
 *   1. 每 1 秒向 K210 发送心跳帧 "$alive#"
 *   2. 接收到 K210 发来的帧后，在 OLED 显示 payload
 *   3. 将收到的帧原样回传（echo: "$echo:payload#"）
 *
 * OLED 128×32, Font_7x10 layout:
 *   Line 0: "RX: hello"        (最近收到的 payload)
 *   Line 1: "TX: alive"        (最近发送的内容)
 */

#include "test_k210_comm.h"
#include "k210_comm.h"
#include "oled.h"

#define HEARTBEAT_INTERVAL_MS  1000    /* 心跳发送间隔 */
#define DISPLAY_INTERVAL_MS    200     /* OLED 刷新间隔 */

static uint16_t heartbeat_cnt = 0;
static uint16_t disp_cnt     = 0;
static uint8_t  new_rx_show  = 0;     /* 1 = 有新接收需要显示 */

static char rx_display[17] = "none";   /* 显示用，最多 16 字符 */
static char tx_display[17] = "alive";  /* 最近发送内容 */

/* 截断字符串到 max_len 字符，加 null */
static void truncate_str(const char *src, char *dst, uint8_t max_len)
{
    uint8_t i;
    for (i = 0; i < max_len && src[i] != '\0'; i++) {
        dst[i] = src[i];
    }
    dst[i] = '\0';
}

void TestK210Comm_Init(void)
{
    heartbeat_cnt = 0;
    disp_cnt      = 0;
    new_rx_show   = 0;

    OLED_Clear();
    OLED_GotoXY(0, 0);
    OLED_Puts("RX: none", &Font_7x10, OLED_COLOR_WHITE);
    OLED_GotoXY(0, 10);
    OLED_Puts("TX: alive", &Font_7x10, OLED_COLOR_WHITE);
    OLED_Update();
}

void TestK210Comm_Tick(void)
{
    /* ---- 接收处理 ---- */
    if (K210Comm_HasMessage()) {
        const char *msg = K210Comm_GetMessage();
        K210Comm_ClearFlag();

        /* 截断保存用于显示 */
        truncate_str(msg, rx_display, 16);
        new_rx_show = 1;

        /* Echo 回传: "$echo:payload#" */
        char echo_buf[24] = "echo:";
        uint8_t pos = 5;
        while (msg[pos - 5] != '\0' && pos < 23) {
            echo_buf[pos] = msg[pos - 5];
            pos++;
        }
        echo_buf[pos] = '\0';
        K210Comm_SendFrame(echo_buf);
        truncate_str(echo_buf, tx_display, 16);
    }

    /* ---- 心跳发送（每 1 秒） ---- */
    if (++heartbeat_cnt >= HEARTBEAT_INTERVAL_MS) {
        heartbeat_cnt = 0;
        K210Comm_SendFrame("alive");
    }

    /* ---- OLED 显示 ---- */
    if (new_rx_show || ++disp_cnt >= DISPLAY_INTERVAL_MS) {
        disp_cnt      = 0;
        new_rx_show   = 0;

        OLED_Clear();

        /* Line 0: "RX: xxxxx" */
        OLED_GotoXY(0, 0);
        OLED_Puts("RX: ", &Font_7x10, OLED_COLOR_WHITE);
        OLED_Puts(rx_display, &Font_7x10, OLED_COLOR_WHITE);

        /* Line 1: "TX: xxxxx" */
        OLED_GotoXY(0, 10);
        OLED_Puts("TX: ", &Font_7x10, OLED_COLOR_WHITE);
        OLED_Puts(tx_display, &Font_7x10, OLED_COLOR_WHITE);

        OLED_Update();
    }
}
