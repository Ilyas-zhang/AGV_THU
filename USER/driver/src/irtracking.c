/*
 * irtracking.c — 四路红外循迹传感器驱动
 *
 *   四路循迹传感器布局（从小车前进方向看）：
 *
 *   左 ←————————————→ 右
 *        X1    X2    X3    X4
 *
 *   电平约定：检测到黑线 → 低电平(0)，白色地面 → 高电平(1)
 *
 *   非对称去抖滤波（IRTracking_Tick @ 1 kHz 驱动）：
 *     - 检测到黑线 (LOW)：立即触发，filtered = 0
 *     - 检测到白地 (HIGH)：需连续 IRTRACK_RELEASE 次才释放，filtered = 1
 *     防止红外接收管释放慢导致的"stuck LOW"问题。
 */

#include "irtracking.h"
#include "main.h"

/* ---- private state (per channel) ---- */

static uint8_t  filtered[4]    = {1, 1, 1, 1};   /* 滤波后输出: 0=黑线 1=白地, 初始全白 */
static uint8_t  clear_cnt[4]   = {0, 0, 0, 0};   /* 连续读到 HIGH 的计数 */

/* ---- raw GPIO read for each channel ---- */

static inline uint8_t read_raw(uint8_t ch)
{
    switch (ch) {
    case 0: return (HAL_GPIO_ReadPin(X1_GPIO_Port, X1_Pin) == GPIO_PIN_RESET) ? 0 : 1;
    case 1: return (HAL_GPIO_ReadPin(X2_GPIO_Port, X2_Pin) == GPIO_PIN_RESET) ? 0 : 1;
    case 2: return (HAL_GPIO_ReadPin(X3_GPIO_Port, X3_Pin) == GPIO_PIN_RESET) ? 0 : 1;
    case 3: return (HAL_GPIO_ReadPin(X4_GPIO_Port, X4_Pin) == GPIO_PIN_RESET) ? 0 : 1;
    default: return 1;
    }
}

/* ========== 初始化 ========== */

void IRTracking_Init(void)
{
    /* GPIO 已由 CubeMX 配置为 Input，无需额外初始化 */

    /* 重置滤波状态 */
    for (uint8_t i = 0; i < 4; i++) {
        filtered[i]  = 1;   /* 初始假设全白 */
        clear_cnt[i] = 0;
    }
}

/* ========== 1 ms tick: 采样 + 非对称去抖 ========== */

void IRTracking_Tick(void)
{
    for (uint8_t i = 0; i < 4; i++) {
        uint8_t raw = read_raw(i);

        if (raw == 0) {
            /* 检测到黑线 → 立即触发 */
            filtered[i]  = 0;
            clear_cnt[i] = 0;
        } else {
            /* 读到白地 → 需连续 N 次才释放 */
            if (++clear_cnt[i] >= IRTRACK_RELEASE) {
                filtered[i]  = 1;
                clear_cnt[i] = 0;
            }
        }
    }
}

/* ========== 读取接口（返回滤波后状态） ========== */

uint8_t IRTracking_Read(uint8_t channel)
{
    if (channel > 3) return 1;
    return filtered[channel];
}

uint8_t IRTracking_ReadAll(void)
{
    uint8_t state = 0;
    if (filtered[0]) state |= 0x01;  /* bit0 = X1 */
    if (filtered[1]) state |= 0x02;  /* bit1 = X2 */
    if (filtered[2]) state |= 0x04;  /* bit2 = X3 */
    if (filtered[3]) state |= 0x08;  /* bit3 = X4 */
    return state;
}
