#ifndef __FOLLOW_VISION_CONFIG_H
#define __FOLLOW_VISION_CONFIG_H

/*
 * 循迹+视觉综合任务参数
 *
 * 逻辑：循迹行驶 → K210 路牌指令 → 按指令执行动作 → 恢复循迹
 * L/R 路牌触发环岛通行（不使用 overtake 模块，直接 PWM 控制）
 */

/* ---- 循迹参数 ---- */
#define FV_LINE_BASE_SPEED      2850    /* 初始正常循迹直行速度 */
#define FV_LINE_SLOW_SPEED      2100    /* 限速时循迹降速 */
#define FV_LINE_RELEASE_SPEED   2300    /* 解除限速后循迹速度 */

/* ---- 环岛通行参数 ---- */
#define FV_ISLAND_DELAY_MS      250     /* 延缓期时长 ms（识别到路牌后直行接近入口） */
#define FV_ISLAND_RUN_MS        250     /* 岛内循迹时长 ms */
#define FV_ISLAND_FWD_SPEED     2300    /* 延缓期直行速度 (≥2100) */
#define FV_ISLAND_ROT_SPEED     3000    /* 原地旋转找线速度 (≥2100) */
#define FV_ISLAND_FOLLOW_SPEED  2300    /* 岛内循迹速度 (≥2100) */

/* ---- 鸣笛自动关闭 ---- */
#define FV_HORN_MS              1000    /* 蜂鸣器响持续时间 ms */

/* ---- K210 心跳 ---- */
#define FV_K210_HEARTBEAT_MS    1000    /* STM32 发心跳间隔 ms */

/* ---- OLED 调试 ---- */
#define FV_OLED_ENABLE          0       /* 1=OLED 实时显示, 0=禁用 */
#define FV_DISPLAY_MS           80      /* OLED 刷新间隔 ms */

#endif /* __FOLLOW_VISION_CONFIG_H */
