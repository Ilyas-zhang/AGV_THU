#ifndef __OBSTACLE_AVOID_CONFIG_H
#define __OBSTACLE_AVOID_CONFIG_H

/*
 * 超声避障参数配置
 *
 * 调参方法：
 *   1. 先调阈值：WARN/STOP/SAFE 决定触发距离
 *   2. 再调时间：BACKUP/TURN/STOP_DELAY 决定动作持续
 *   3. 最后调速度：各状态下的 PWM 占空比
 */

/* ---- 距离阈值 (mm) ---- */
#define OA_WARN_DIST_MM          300     /* 减速阈值 30cm */
#define OA_STOP_DIST_MM          150     /* 停车阈值 15cm */
#define OA_SAFE_DIST_MM          400     /* 恢复全速阈值 40cm */

/* ---- 动作时间 (ms) ---- */
#define OA_BACKUP_MS             800     /* 后退持续时间 */
#define OA_TURN_MS               600     /* 转向持续时间 */
#define OA_STOP_DELAY_MS         500     /* 停车停留时间 */

/* ---- 超声触发 ---- */
#define OA_TRIGGER_MS            60      /* 触发间隔 ms */

/* ---- 速度 (0~3599) ---- */
#define OA_FORWARD_SPEED         1800    /* 前进速度 */
#define OA_SLOW_SPEED            900     /* 减速速度 */
#define OA_BACKUP_SPEED          900     /* 后退速度 */
#define OA_TURN_SPEED            900     /* 转向速度 */

/* ---- 转向方向 ---- */
#define OA_TURN_DIR              1       /* 1=右旋, -1=左旋 */

#endif /* __OBSTACLE_AVOID_CONFIG_H */
