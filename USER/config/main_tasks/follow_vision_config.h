#ifndef __FOLLOW_VISION_CONFIG_H
#define __FOLLOW_VISION_CONFIG_H

/*
 * 循迹+视觉综合任务参数
 *
 * 逻辑：循迹行驶 → K210 路牌指令 → 按指令执行动作 → 恢复循迹
 * 超车机动参数独立配置（覆盖 overtake_config.h 默认值）
 */

/* ---- 循迹参数 ---- */
#define FV_LINE_BASE_SPEED      2850    /* 正常循迹直行速度 */
#define FV_LINE_SLOW_SPEED      2100    /* 限速时循迹降速 */

/* ---- 两次超车间直行 ---- */
#define FV_FWD_WAIT_SPEED       2300    /* 两次超车间直行速度 (≥2100) */
#define FV_FWD_WAIT_MS          2000    /* 两次超车间直行时间 ms */

/* ---- 脱线搜索（超车完成后找黑线） ---- */
#define FV_SEARCH_SPEED         2800    /* 原地旋转找线速度 (≥2100) */
#define FV_SEARCH_TIMEOUT_MS    3000    /* 搜索超时 ms，超时后放弃进入循迹 */

/* ---- 鸣笛自动关闭 ---- */
#define FV_HORN_MS              1000    /* 蜂鸣器响持续时间 ms */

/* ---- K210 心跳 ---- */
#define FV_K210_HEARTBEAT_MS    1000    /* STM32 发心跳间隔 ms */

/* ---- 视觉超车机动参数（覆盖 overtake_config.h 默认值） ----
 * 第一次超车（OVT1）和第二次超车（OVT2）参数独立，
 * 因为左右旋可能因物理不对称需要不同时间/速度。
 * 方向由路牌指令决定（LEFT→左超车, RIGHT→右超车），
 * 第二次超车方向自动取反。
 *
 * 这些参数在触发超车前通过 Overtake_SetConfig() 注入，
 * 不影响其他使用 overtake.c 的任务。
 *
 * ⚠ 电机 PWM < 2100 无法克服静摩擦，车轮不转
 */

/* 第一次超车（由路牌方向决定） */
#define FV_OVT1_STOP_DELAY_MS   1000    /* 停车等待时间 ms */
#define FV_OVT1_ROTATE_MS       500     /* 原地旋转时间 ms（调到≈90°） */
#define FV_OVT1_PASS_MS         1000    /* 直行超越时间 ms */
#define FV_OVT1_ROTATE_SPEED    3000    /* 原地旋转速度 (≥2100) */
#define FV_OVT1_PASS_SPEED      2300    /* 直行超越速度 (≥2100) */

/* 第二次超车（方向取反，回到原车道） */
#define FV_OVT2_STOP_DELAY_MS   1000    /* 停车等待时间 ms */
#define FV_OVT2_ROTATE_MS       480     /* 原地旋转时间 ms（调到≈90°） */
#define FV_OVT2_PASS_MS         1100    /* 直行超越时间 ms */
#define FV_OVT2_ROTATE_SPEED    3000    /* 原地旋转速度 (≥2100) */
#define FV_OVT2_PASS_SPEED      2300    /* 直行超越速度 (≥2100) */

/* ---- OLED 调试 ---- */
#define FV_OLED_ENABLE          0       /* 1=OLED 实时显示, 0=禁用 */
#define FV_DISPLAY_MS           80      /* OLED 刷新间隔 ms */

#endif /* __FOLLOW_VISION_CONFIG_H */
