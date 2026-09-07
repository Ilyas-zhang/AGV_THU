#ifndef __VISION_LINE_FOLLOW_CONFIG_H
#define __VISION_LINE_FOLLOW_CONFIG_H

/*
 * 视觉循迹参数配置
 *
 * K210 发送线偏差误差 $e<value># (±100 或 999=脱线)
 * STM32 经 EMA 滤波后做 Kp 比例差速
 *
 * left_speed  = base + Kp * filtered_error
 * right_speed = base - Kp * filtered_error
 * 快侧 ≥ MIN_SPEED（驱动侧必须能转）
 * 慢侧 ≥ 0（允许停转增大差速）
 */

/* ---- 速度 (0~3599) ---- */
#define VLF_BASE_SPEED          2500    /* 直行基准速度 */
#define VLF_MIN_SPEED           2100    /* 快侧最低速度（电机启动门槛） */
#define VLF_MAX_SPEED           3599    /* PWM 最大值 */

/* ---- 比例增益 ---- */
#define VLF_KP                  15      /* Kp: PWM 差速/误差单位 */
/* Kp=15, error=100 → adj=1500, 差速可达 3000 */

/* ---- 误差 EMA 滤波 ---- */
#define VLF_EMA_ALPHA           4       /* EMA 系数: α=1/N, N=4 → α=0.25 */

/* ---- 脱线旋转 ---- */
#define VLF_ROTATE_SPEED       2500    /* 脱线找回旋转速度 */

/* ---- 通讯 ---- */
#define VLF_TIMEOUT_MS          500     /* 无 K210 消息超时 (ms)，超时后旋转 */
#define VLF_HEARTBEAT_MS        1000    /* STM32 发心跳间隔 (ms) */

/* ---- OLED 显示 ---- */
#define VLF_DISPLAY_MS          50      /* OLED 刷新间隔 ms */

#endif /* __VISION_LINE_FOLLOW_CONFIG_H */
