#ifndef __LED_H
#define __LED_H

#include <stdint.h>

/**
 * @brief  Initialize LED driver, turn off all LEDs.
 */
void LED_Init(void);

/**
 * @brief  Set left and right RGB LED parameters.
 * @param  l_r     Left  Red   intensity 0~100 (0=off, 100=full brightness)
 * @param  l_g     Left  Green intensity 0~100
 * @param  l_b     Left  Blue  intensity 0~100
 * @param  l_blink Left  blink mode: 0=steady, 1=slow blink (1Hz), 2=fast blink (5Hz)
 * @param  r_r     Right Red   intensity 0~100
 * @param  r_g     Right Green intensity 0~100
 * @param  r_b     Right Blue  intensity 0~100
 * @param  r_blink Right blink mode: 0=steady, 1=slow blink (1Hz), 2=fast blink (5Hz)
 */
void LED_Set(uint8_t l_r, uint8_t l_g, uint8_t l_b, uint8_t l_blink,
             uint8_t r_r, uint8_t r_g, uint8_t r_b, uint8_t r_blink);

/**
 * @brief  1 ms tick handler for software PWM and blink.
 *         Must be called from SysTick_Handler every 1 ms.
 */
void LED_Tick(void);

#endif /* __LED_H */
