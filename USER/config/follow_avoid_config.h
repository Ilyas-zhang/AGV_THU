#ifndef __FOLLOW_AVOID_CONFIG_H
#define __FOLLOW_AVOID_CONFIG_H

/*
 * 循迹避障综合任务参数
 *
 * 逻辑：循迹 → 障碍物 → 停车 → 左超车 → 等 → 右超车 → 恢复循迹
 * 超车机动参数在 overtake_config.h 中调节
 */

/* ---- 超声触发 ---- */
#define FA_ULTRASONIC_TRIGGER_MS   60      /* 超声测距触发间隔 ms (HC-SR04 ≥60ms) */

/* ---- 超声阈值 (mm) ---- */
#define FA_WARN_DIST            300     /* 减速阈值 30cm：循迹降速 */
#define FA_STOP_DIST            150     /* 停车阈值 15cm：触发避障 */

/* ---- 避障时序 (ms) ---- */
#define FA_STOP_DELAY_MS        500     /* 停车等待时间 */
#define FA_OVT_GAP_MS           2000     /* 两次超车间直行时间（超过障碍物） */

/* ---- 循迹参数 ---- */
#define FA_LINE_BASE_SPEED      2850    /* 正常循迹直行速度 */
#define FA_LINE_SLOW_SPEED      2100    /* 障碍警告时循迹降速 */

/* ---- 两次超车间直行 ---- */
#define FA_FWD_WAIT_SPEED       2300    /* 两次超车间直行速度 (≥2100) */

/* ---- 避障超车机动参数（覆盖 overtake_config.h 默认值） ----
 * 第一次超车（OVT1）和第二次超车（OVT2）参数独立，
 * 因为左右旋可能因物理不对称需要不同时间/速度。
 * 方向也在此配置，不硬编码。
 *
 * 这些参数在触发超车前通过 Overtake_SetConfig() 注入，
 * 不影响其他使用 overtake.c 的任务。
 *
 * ⚠ 电机 PWM < 2100 无法克服静摩擦，车轮不转
 */

/* 第一次超车（默认：右超车） */
#define FA_OVT1_DIR               -1     /* -1=OVERTAKE_DIR_RIGHT, 1=OVERTAKE_DIR_LEFT */
#define FA_OVT1_STOP_DELAY_MS   1000    /* 停车等待时间 ms */
#define FA_OVT1_ROTATE_MS         500    /* 原地旋转时间 ms（调到≈90°） */
#define FA_OVT1_PASS_MS          1000    /* 直行超越时间 ms */
#define FA_OVT1_ROTATE_SPEED     3000    /* 原地旋转速度 (≥2100) */
#define FA_OVT1_PASS_SPEED       2300    /* 直行超越速度 (≥2100) */

/* 第二次超车（默认：左超车，回到原车道） */
#define FA_OVT2_DIR                1     /* 1=OVERTAKE_DIR_LEFT, -1=OVERTAKE_DIR_RIGHT */
#define FA_OVT2_STOP_DELAY_MS   1000    /* 停车等待时间 ms */
#define FA_OVT2_ROTATE_MS         480    /* 原地旋转时间 ms（调到≈90°） */
#define FA_OVT2_PASS_MS          1100    /* 直行超越时间 ms */
#define FA_OVT2_ROTATE_SPEED     3000    /* 原地旋转速度 (≥2100) */
#define FA_OVT2_PASS_SPEED       2300    /* 直行超越速度 (≥2100) */

/* ---- OLED 调试 ---- */
#define FA_OLED_ENABLE          0       /* 1=OLED 实时显示, 0=禁用 */
#define FA_DISPLAY_MS           80      /* OLED 刷新间隔 ms */

#endif /* __FOLLOW_AVOID_CONFIG_H */
