/*
 * follow_vision.h — 循迹 + 视觉路牌综合任务
 *
 * 正常循迹行驶，K210 YOLOv2 路牌识别触发驾驶行为：
 *   LEFT  → 左超车 → 直行 → 右超车 → 恢复循迹
 *   RIGHT → 右超车 → 直行 → 左超车 → 恢复循迹
 *   HORN  → 鸣笛（自动关闭）
 *   SPEED_LIMIT  → 循迹降速
 *   SPEED_RELEASE→ 恢复正常循迹速度
 *   PARK1/PARK2 (1/2) → 停车
 *
 * Call FollowVision_Init() once, FollowVision_Tick() from SysTick every 1 ms.
 * Requires K210Comm_Init() called beforehand for RXNE interrupt.
 */

#ifndef __FOLLOW_VISION_H
#define __FOLLOW_VISION_H

#include <stdint.h>

void FollowVision_Init(void);
void FollowVision_Tick(void);

/* ---- 调试接口 ---- */

/**
 * @brief  Get current state name (for OLED display).
 * @retval "FOLLOW", "OVT1", "FWD", "OVT2", "SEARCH", "SLOW", "PARK", "HORN"
 */
const char *FollowVision_GetStateName(void);

/**
 * @brief  Get last received K210 sign payload.
 * @retval "L","R","H","W","F","1","2", or "-" if none.
 */
const char *FollowVision_GetLastSign(void);

#endif /* __FOLLOW_VISION_H */
