/*
 * test_obstacle.h — Ultrasonic obstacle avoidance test task
 *
 * Initializes OLED + ultrasonic + avoidance state machine,
 * displays distance and state on OLED.
 * Call TestObstacle_Tick() from SysTick every 1 ms.
 */

#ifndef __TEST_OBSTACLE_H
#define __TEST_OBSTACLE_H

void TestObstacle_Init(void);
void TestObstacle_Tick(void);

#endif /* __TEST_OBSTACLE_H */
