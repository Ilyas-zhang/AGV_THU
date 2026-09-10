#ifndef __YOLO_DRIVE_CONFIG_H
#define __YOLO_DRIVE_CONFIG_H

/*
 * YOLOv2 视觉驾驶参数配置 — 7 标识版
 *
 * 标识映射 (YOLOv2 7 类):
 *   H (鸣笛)           → 蜂鸣器响 (自动关闭)
 *   L (左转)           → 左超车 → 直行 → 右超车 → 恢复
 *   P1 (停车位类型1)   → 停车
 *   P2 (停车位类型2)   → 停车
 *   R (右转)           → 右超车 → 直行 → 左超车 → 恢复
 *   W (限速)           → 降速行驶
 *   F (解除限速)       → 恢复正常速度
 *
 * 超车动作参数在 overtake_config.h 中统一管理。
 */

/* ---- 速度档位 (0~3599, 必须 ≥ 2100) ---- */
#define YD_FORWARD_SPEED         2300    /* 正常前进速度 */
#define YD_SLOW_SPEED            2100    /* 限速前进速度 */

/* ---- 超车间直行时间 (ms) ---- */
#define YD_FWD_WAIT_MS           1000    /* 两次超车之间的直行时间 */

/* ---- 鸣笛自动关闭 (ms) ---- */
#define YD_HORN_MS               1000    /* 蜂鸣器响持续时间 */

#endif /* __YOLO_DRIVE_CONFIG_H */
