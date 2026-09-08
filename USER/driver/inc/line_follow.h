/*
 * line_follow.h — 四路循迹高速平滑控制
 *
 * 策略：
 *   1) 直行 / 左转 / 右转三个主状态，减少动作切换；
 *   2) 圆弧使用轻反转内侧 + 高速前转外侧；
 *   3) 直角/大偏差自动升级为强双向差速；
 *   4) 转弯带最小保持、中心确认和反方向确认，抑制左右抽搐；
 *   5) 中心稳定后才恢复直行，保持“先转正，再直行”；
 *   6) 正反换向强制经过 0，兼顾高速响应和机械平顺性。
 */

#ifndef __LINE_FOLLOW_H
#define __LINE_FOLLOW_H

#include <stdint.h>

void LineFollow_Init(void);

/**
 * @brief  运行一次循迹控制，设计调用周期为 1 ms。
 * @param  base_speed  直行 PWM，范围 0~3599。
 */
void LineFollow_Run(int16_t base_speed);

/**
 * @brief  最近一次检测到的黑线位置误差。
 * @retval -3~-1: 黑线在车体左侧；0: 居中/对称；+1~+3: 黑线在车体右侧。
 */
int8_t LineFollow_GetError(void);

/**
 * @brief  当前运动状态。
 * @retval -1: 物理左转；0: 直行；+1: 物理右转。
 */
int8_t LineFollow_GetTurnDirection(void);

#endif /* __LINE_FOLLOW_H */
