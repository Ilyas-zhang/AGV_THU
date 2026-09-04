/*
 * test_ultrasonic_overtake.h — Ultrasonic overtaking test task
 *
 * Initializes ultrasonic + overtaking state machine + OLED display.
 * Call TestUltrasonicOvertake_Tick() from SysTick every 1 ms.
 */

#ifndef __TEST_ULTRASONIC_OVERTAKE_H
#define __TEST_ULTRASONIC_OVERTAKE_H

void TestUltrasonicOvertake_Init(void);
void TestUltrasonicOvertake_Tick(void);

#endif /* __TEST_ULTRASONIC_OVERTAKE_H */
