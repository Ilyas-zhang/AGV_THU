#ifndef __IR_AVOID_DRIVE_CONFIG_H
#define __IR_AVOID_DRIVE_CONFIG_H

/*
 * 红外避障驾驶参数配置
 *
 * 行为：
 *   仅左障碍 → 右超车（先右旋后左旋）
 *   仅右障碍 → 左超车（先左旋后右旋）
 *   双侧障碍 → 左超车
 *   无障碍   → 前进
 *
 * 超车动作参数在 overtake_config.h 中统一管理。
 */

/* ---- 前进速度 ---- */
#define IAD_FORWARD_SPEED        2300    /* 前进速度 */

/* ---- 超车方向 ---- */
#define IAD_BOTH_OVERTAKE_DIR    OVERTAKE_DIR_LEFT  /* 双侧检测时的超车方向 */

#endif /* __IR_AVOID_DRIVE_CONFIG_H */
