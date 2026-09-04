/*
 * test_k210_comm.h — K210 通讯测试任务
 *
 * 在 OLED 上显示接收到的帧内容，
 * 同时每 1 秒向 K210 发送心跳帧。
 * Call TestK210Comm_Init() once, then TestK210Comm_Tick() every 1 ms.
 */

#ifndef __TEST_K210_COMM_H
#define __TEST_K210_COMM_H

void TestK210Comm_Init(void);
void TestK210Comm_Tick(void);

#endif /* __TEST_K210_COMM_H */
