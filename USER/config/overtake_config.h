#ifndef __OVERTAKE_CONFIG_H
#define __OVERTAKE_CONFIG_H

/*
 * 超车机动参数配置
 *
 * 超车动作：
 *   左超车: 停车 → 原地左旋90° → 直行 → 原地右旋90° → 完成
 *   右超车: 停车 → 原地右旋90° → 直行 → 原地左旋90° → 完成
 *
 * 调参方法：
 *   1. 调时间：STOP_DELAY / ROTATE_MS / PASS_MS
 *   2. 调速度：各状态下 PWM 占空比（必须 ≥ 2100）
 *   3. 调角度：ROTATE_MS 过大/过小 → 角度偏离90°
 *
 * ⚠ 电机 PWM < 2100 无法克服静摩擦，车轮不转
 */

/* ---- 动作时间 (ms) ---- */
#define OV_STOP_DELAY_MS         1000    /* 停车等待时间 */
#define OV_ROTATE_MS             500     /* 原地旋转时间（调到≈90°） */
#define OV_PASS_MS               500     /* 直行时间 */

/* ---- 速度 (0~3599, 必须 ≥ 2100) ---- */
#define OV_ROTATE_SPEED          3000    /* 原地旋转速度（双侧反向差速） */
#define OV_PASS_SPEED            2300    /* 直行超越速度 */

/* ---- 超车方向 ---- */
#define OVERTAKE_DIR_LEFT        1       /* 左超车：先左旋后右旋 */
#define OVERTAKE_DIR_RIGHT       (-1)    /* 右超车：先右旋后左旋 */

#endif /* __OVERTAKE_CONFIG_H */
