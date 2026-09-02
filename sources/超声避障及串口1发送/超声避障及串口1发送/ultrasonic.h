/*
 * ultrasonic.h
 *
 *  Created on: Aug 21, 2024
 *      Author: DPY
 */

#ifndef ULTRASONIC_H_
#define ULTRASONIC_H_
#include "main.h"
#include "tim.h"

void ultrasonic_init(void);
float Get_distance(void);
void Delay_US(uint32_t nus);

#endif /* ULTRASONIC_H_ */
