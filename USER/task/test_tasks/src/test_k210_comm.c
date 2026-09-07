/*
 * test_k210_comm.c — K210 路牌识别 + 视觉驾驶测试
 *
 * 薄封装：调用 VisionDrive 驱动 + OLED 状态显示
 *
 * K210 (sign_detect.py) 发送路牌指令：
 *   "$R#" → RIGHT   "$L#" → LEFT   "$S#" → STOP
 *
 * 视觉驾驶行为（由 vision_drive.c 驱动）：
 *   LEFT  → 左超车 → 直行1s → 右超车 → 恢复前进
 *   RIGHT → 右超车 → 直行1s → 左超车 → 恢复前进
 *   STOP  → 停车
 *
 * USART2: PD5(TX)/PD6(RX) 115200 8N1
 * OLED 128×32, Font_7x10, 20 Hz 刷新 (I2C 400kHz)
 */

#include "test_k210_comm.h"
#include "vision_drive.h"
#include "oled.h"
#include "led.h"
#include "led_config.h"
#include "k210_comm.h"
#include "main.h"
#include "usart.h"

#define DISPLAY_INTERVAL_MS    50      /* OLED 刷新间隔 (I2C 400kHz 下 ~12ms/帧, 50ms→20Hz) */
#define LED_BLINK_MS           500     /* LED 闪烁半周期 */

static uint16_t disp_cnt = 0;
static uint16_t led_cnt  = 0;

/* ---- helpers ---- */

static const char *sign_name(char c)
{
    switch (c) {
    case 'R': return "RIGHT ";
    case 'L': return "LEFT  ";
    case 'S': return "STOP  ";
    case 'a': return "ALIVE ";
    default:  return "---   ";
    }
}

/* ---- Init ---- */

void TestK210Comm_Init(void)
{
    disp_cnt = 0;
    led_cnt  = 0;

    /* K210Comm_Init() 已在 main() 中调用并使能 RXNE 中断 */
    VisionDrive_Init();

    OLED_Clear();
    OLED_GotoXY(0, 0);
    OLED_Puts("VisDrive IDLE", &Font_7x10, OLED_COLOR_WHITE);
    OLED_GotoXY(0, 10);
    OLED_Puts("Sign: ---", &Font_7x10, OLED_COLOR_WHITE);
    OLED_Update();
}

/* ---- Tick ---- */

void TestK210Comm_Tick(void)
{
    /* ---- Drive vision + overtake state machine ---- */
    VisionDrive_Tick();

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
        /* 心跳 */
        K210Comm_SendFrame("alive");
    }

    /* ---- OLED 显示 ---- */
    if (++disp_cnt >= DISPLAY_INTERVAL_MS) {
        disp_cnt = 0;

        OLED_Clear();

        /* Line 0: "VisDrive ROT1" — 当前驾驶状态 */
        OLED_GotoXY(0, 0);
        OLED_Puts("VisDrive ", &Font_7x10, OLED_COLOR_WHITE);
        OLED_Puts(VisionDrive_GetDetailStateName(), &Font_7x10, OLED_COLOR_WHITE);

        /* Line 1: "Sign: LEFT " — 最近路牌 */
        OLED_GotoXY(0, 10);
        OLED_Puts("Sign:", &Font_7x10, OLED_COLOR_WHITE);
        OLED_Puts(sign_name(VisionDrive_GetLastSign()), &Font_7x10, OLED_COLOR_WHITE);

        OLED_Update();
    }
}
