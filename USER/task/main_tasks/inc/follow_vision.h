/*
 * follow_vision.h — 循迹 + 视觉路牌综合任务
 *
 * 正常循迹行驶，K210 YOLOv2 路牌识别触发驾驶行为：
 *   LEFT  → 延缓直行 → 左旋转入岛 → 岛内循迹 → 左旋转出岛 → 恢复循迹
 *   RIGHT → 延缓直行 → 右旋转入岛 → 岛内循迹 → 右旋转出岛 → 恢复循迹
 *   HORN  → 鸣笛（自动关闭）
 *   SPEED_LIMIT  → 循迹降速
 *   SPEED_RELEASE→ 恢复循迹 (速度 FV_LINE_RELEASE_SPEED)
 *   PARK1/PARK2 (1/2) → 停车
 *
 * Call FollowVision_Init() once, FollowVision_Tick() from SysTick every 1 ms.
 * Requires K210Comm_Init() called beforehand for RXNE interrupt.
 *
 * 不使用 overtake 模块，环岛通行直接 PWM 控制。
 */

#ifndef __FOLLOW_VISION_H
#define __FOLLOW_VISION_H

#include <stdint.h>

void FollowVision_Init(void);
void FollowVision_Tick(void);

/* ---- 调试接口 ---- */

/**
 * @brief  Get current state name (for OLED display).
 * @retval "FOLLOW", "ISL_DLY", "ISL_R1", "ISL_FLW", "ISL_R2", "SLOW", "PARK", "HORN"
 */
const char *FollowVision_GetStateName(void);

/**
 * @brief  Get last received K210 sign payload.
 * @retval "L","R","H","W","F","1","2", or "-" if none.
 */
const char *FollowVision_GetLastSign(void);

#endif /* __FOLLOW_VISION_H */
