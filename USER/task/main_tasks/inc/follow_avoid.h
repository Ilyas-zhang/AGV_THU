/*
 * follow_avoid.h — 循迹 + 超声避障综合任务
 *
 * 正常循迹行驶，障碍物靠近时自动降速→停车→后退→转弯绕开→恢复循迹。
 * 状态优先级：避障 > 循迹
 *
 * Call FollowAvoid_Init() once, FollowAvoid_Tick() from SysTick every 1 ms.
 */

#ifndef __FOLLOW_AVOID_H
#define __FOLLOW_AVOID_H

#include <stdint.h>

void FollowAvoid_Init(void);
void FollowAvoid_Tick(void);

/* ---- 调试接口 ---- */

/**
 * @brief  Get current state name (for OLED display).
 * @retval "FOLLOW", "WARN", "STOP", "BACK", "TURN", "FWD", "RESUME"
 */
const char *FollowAvoid_GetStateName(void);

/**
 * @brief  Get last ultrasonic distance reading (mm).
 */
uint16_t FollowAvoid_GetDistance(void);

#endif /* __FOLLOW_AVOID_H */
