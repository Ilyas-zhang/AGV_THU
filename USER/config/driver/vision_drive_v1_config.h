#ifndef __VISION_DRIVE_V1_CONFIG_H
#define __VISION_DRIVE_V1_CONFIG_H

/*
 * 视觉驾驶参数配置 — 9 标识版 [LEGACY]
 *
 * ⚠ 旧版驱动配置，基于 K210 自学习分类器 (9 类)。
 * 新版 YOLOv2 目标检测 (7 类) 配置见 yolo_drive_config.h。
 *
 * 标识映射：
 *   L (左转)       → 左超车 → 直行 → 右超车 → 恢复
 *   R (右转)       → 右超车 → 直行 → 左超车 → 恢复
 *   H (鸣笛)       → 蜂鸣器响
 *   W (限速)       → 降速行驶
 *   F (解除限速)   → 恢复正常速度
 *   D (红灯)       → 停车
 *   Y (黄灯)       → 停车
 *   G (绿灯)       → 通行/恢复
 *   B (倒车入库)   → 倒车
 *
 * 超车动作参数在 overtake_config.h 中统一管理。
 */

/* ---- 速度档位 (0~3599, 必须 ≥ 2100) ---- */
#define VD_FORWARD_SPEED         2300    /* 正常前进速度 */
#define VD_SLOW_SPEED            2100    /* 限速前进速度 */
#define VD_BACK_SPEED            2300    /* 倒车速度 */

/* ---- 超车间直行时间 (ms) ---- */
#define VD_FWD_WAIT_MS           1000    /* 两次超车之间的直行时间 */

#endif /* __VISION_DRIVE_V1_CONFIG_H */
