/*
 * test_encoder.h — Encoder test with OLED display
 *
 * Displays 4-channel encoder delta counts on OLED.
 * Call TestEncoder_Init() once, then TestEncoder_Tick() every 1 ms.
 */

#ifndef __TEST_ENCODER_H
#define __TEST_ENCODER_H

void TestEncoder_Init(void);
void TestEncoder_Tick(void);

#endif /* __TEST_ENCODER_H */
