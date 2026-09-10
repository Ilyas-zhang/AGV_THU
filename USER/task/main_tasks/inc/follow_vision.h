/*
 * follow_vision.h — 循迹 + 视觉路牌综合任务
 *
 * 入环岛：三层保险
 *   K210 只提前发送方向并打开转向灯；
 *   编码器走过最小距离后才"解锁"路口检测；
 *   真正到路口以后 FollowVision 暂时独占电机：
 *
 *     正常循迹
 *       -> 收到 L/R，亮灯但继续循迹
 *       -> 编码器空间门控解锁
 *       -> 方向相关宽路口 / 稳定全白被确认
 *       -> 停车 1s
 *       -> 按 L/R 原地转向
 *       -> 先脱旧路口，再等中间传感器稳定抓到新线
 *       -> 岛内循迹
 *       -> 出岛旋转（先脱当前线再找新线）
 *       -> 恢复 FOLLOW / SLOW
 *
 * HORN / SPEED_LIMIT / SPEED_RELEASE / PARK 保持原逻辑。
 *
 * Call FollowVision_Init() once,
 * FollowVision_Tick() from SysTick every 1 ms.
 */

#ifndef __FOLLOW_VISION_H
#define __FOLLOW_VISION_H

#include <stdint.h>

void FollowVision_Init(void);
void FollowVision_Tick(void);

/* ---- 调试接口 ---- */

const char *FollowVision_GetStateName(void);
const char *FollowVision_GetLastSign(void);

/* LEFT/RIGHT 调参辅助：可在调试器里观察 */
uint32_t FollowVision_GetTurnTravelCounts(void);
uint8_t  FollowVision_IsTurnJunctionArmed(void);

#endif /* __FOLLOW_VISION_H */
