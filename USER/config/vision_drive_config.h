#ifndef __VISION_DRIVE_CONFIG_H
#define __VISION_DRIVE_CONFIG_H

/*
 * 视觉驾驶参数配置
 *
 * 行为：
 *   LEFT 路牌  → 左超车 → 直行 → 右超车 → 恢复前进
 *   RIGHT 路牌 → 右超车 → 直行 → 左超车 → 恢复前进
 *   STOP 路牌  → 停车
 *
 * 超车动作参数在 overtake_config.h 中统一管理。
 */

/* ---- 前进速度 (0~3599, 必须 ≥ 2100) ---- */
#define VD_FORWARD_SPEED         2300    /* 正常前进速度 */

/* ---- 超车间直行时间 (ms) ---- */
#define VD_FWD_WAIT_MS           1000    /* 两次超车之间的直行时间 */

#endif /* __VISION_DRIVE_CONFIG_H */
