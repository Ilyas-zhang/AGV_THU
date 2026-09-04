#ifndef __IR_AVOID_DRIVE_CONFIG_H
#define __IR_AVOID_DRIVE_CONFIG_H

/*
 * 红外避障驾驶参数配置
 *
 * 行为：
 *   仅左障碍 → 右转规避
 *   仅右障碍 → 左转规避
 *   双侧障碍 → 先后退再旋转
 *   无障碍   → 前进
 *
 * 调参方法：
 *   1. 先调时间：TURN/BACKUP/ROTATE 决定规避幅度
 *   2. 再调速度：各状态下的 PWM 占空比
 *   3. 旋转方向：ROTATE_DIR = 1 右旋, -1 左旋
 */

/* ---- 动作时间 (ms) ---- */
#define IAD_TURN_MS              400     /* 转向规避持续时间 */
#define IAD_BACKUP_MS            500     /* 后退持续时间 */
#define IAD_ROTATE_MS            300     /* 旋转持续时间 */

/* ---- 速度 (0~3599) ---- */
#define IAD_FORWARD_SPEED        1800    /* 前进速度 */
#define IAD_TURN_SPEED           1200    /* 转向速度 */
#define IAD_BACKUP_SPEED         900     /* 后退速度 */
#define IAD_ROTATE_SPEED         900     /* 旋转速度 */

/* ---- 旋转方向 ---- */
#define IAD_ROTATE_DIR           1       /* 1=右旋, -1=左旋 */

#endif /* __IR_AVOID_DRIVE_CONFIG_H */
