#ifndef __ULTRASONIC_OVERTAKE_CONFIG_H
#define __ULTRASONIC_OVERTAKE_CONFIG_H

/*
 * 超声超车避障参数配置
 *
 * 行为：前方检测到障碍物 → 停车1秒 → 左换道(弧线左转) →
 *       前行超越(超声监测) → 右回道(弧线右转) → 恢复前行
 *
 * 调参方法：
 *   1. 先调阈值：STOP 决定停车距离
 *   2. 再调时间：LEFT_SHIFT/RIGHT_SHIFT 决定换道幅度，
 *      PASS_MIN/PASS_TIMEOUT 决定超越时机
 *   3. 最后调速度：各状态下的 PWM 占空比
 *
 * ⚠ 换道转弯是差速弧线（pwm_car_turn_left/right），不是横向平移。
 *   LEFT_SHIFT_MS / RIGHT_SHIFT_MS 需根据地面摩擦和车速实测调整。
 */

/* ---- 距离阈值 (mm) ---- */
#define UO_STOP_DIST_MM          150     /* 停车阈值 15cm */
#define UO_SAFE_DIST_MM          400     /* 安全距离 40cm（超越判定） */

/* ---- 动作时间 (ms) ---- */
#define UO_STOP_DELAY_MS         1000    /* 停车等待时间 */
#define UO_LEFT_SHIFT_MS         500     /* 左换道弧线时长 */
#define UO_RIGHT_SHIFT_MS        500     /* 右回道弧线时长 */
#define UO_PASS_MIN_MS           800     /* 超越最少前行时间（避免假清除） */
#define UO_PASS_TIMEOUT_MS       3000    /* 超越最大前行时间（安全兜底） */

/* ---- 超越判定 ---- */
#define UO_PASS_CLEAR_COUNT      3       /* 连续安全读数判定超越完成 */

/* ---- 超声触发 ---- */
#define UO_TRIGGER_MS            60      /* 触发间隔 ms */

/* ---- 速度 (0~3599) ---- */
#define UO_FORWARD_SPEED         1800    /* 前进速度 */
#define UO_SHIFT_SPEED           1200    /* 换道转弯速度 */
#define UO_PASS_SPEED            1500    /* 超越前行速度 */

#endif /* __ULTRASONIC_OVERTAKE_CONFIG_H */
