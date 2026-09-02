/*
 * test_ultrasonic.h — Ultrasonic ranging test with OLED display
 *
 * Periodically triggers HC-SR04 and shows distance on OLED.
 * Call TestUltrasonic_Init() once, then TestUltrasonic_Tick() every 1 ms.
 */

#ifndef __TEST_ULTRASONIC_H
#define __TEST_ULTRASONIC_H

void TestUltrasonic_Init(void);
void TestUltrasonic_Tick(void);

#endif /* __TEST_ULTRASONIC_H */
