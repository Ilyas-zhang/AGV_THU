/*
 * test_ir_remote.h — IR remote control test task
 *
 * Displays NEC decoded key code on OLED.
 * Call TestIRRemote_Init() once, TestIRRemote_Tick() from SysTick every 1 ms.
 */

#ifndef __TEST_IR_REMOTE_H
#define __TEST_IR_REMOTE_H

void TestIRRemote_Init(void);
void TestIRRemote_Tick(void);

#endif /* __TEST_IR_REMOTE_H */
