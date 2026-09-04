/*
 * test_line_follow.h — Line tracking test task
 *
 * Displays 4-channel IR tracking sensor state on OLED,
 * and runs line-follow control when sensors detect black line.
 * Call TestLineFollow_Init() once, then TestLineFollow_Tick() every 1 ms.
 */

#ifndef __TEST_LINE_FOLLOW_H
#define __TEST_LINE_FOLLOW_H

void TestLineFollow_Init(void);
void TestLineFollow_Tick(void);

#endif /* __TEST_LINE_FOLLOW_H */
