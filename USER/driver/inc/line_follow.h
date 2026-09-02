#ifndef __LINE_FOLLOW_H
#define __LINE_FOLLOW_H

#include <stdint.h>

/**
 * @brief  Initialize line-follow task (IR tracking + motor).
 */
void LineFollow_Init(void);

/**
 * @brief  Run one iteration of line-follow control.
 *         Call from main loop. Uses IRTracking_ReadAll() to determine
 *         differential drive commands via pwm_car_* API.
 * @param  base_speed  PWM duty 0~3599
 */
void LineFollow_Run(int16_t base_speed);

#endif /* __LINE_FOLLOW_H */
