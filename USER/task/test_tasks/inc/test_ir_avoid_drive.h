/*
 * test_ir_avoid_drive.h — IR avoidance driving test task
 *
 * Initializes IR sensor + avoidance driving state machine + OLED display.
 * Call TestIRAvoidDrive_Tick() from SysTick every 1 ms.
 * IRAvoid_Tick() must also be enabled in SysTick for the debounce filter.
 */

#ifndef __TEST_IR_AVOID_DRIVE_H
#define __TEST_IR_AVOID_DRIVE_H

void TestIRAvoidDrive_Init(void);
void TestIRAvoidDrive_Tick(void);

#endif /* __TEST_IR_AVOID_DRIVE_H */
