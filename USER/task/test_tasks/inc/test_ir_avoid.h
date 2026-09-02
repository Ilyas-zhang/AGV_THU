/*
 * test_ir_avoid.h — IR obstacle avoidance test task
 *
 * Initializes IR sensor + avoidance state machine + OLED display.
 * Call TestIRAvoid_Tick() from SysTick every 1 ms.
 */

#ifndef __TEST_IR_AVOID_H
#define __TEST_IR_AVOID_H

void TestIRAvoid_Init(void);
void TestIRAvoid_Tick(void);

#endif /* __TEST_IR_AVOID_H */
