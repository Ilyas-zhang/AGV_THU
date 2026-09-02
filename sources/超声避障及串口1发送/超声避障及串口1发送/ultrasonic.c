/*
 * ultrasonic.c
 *
 *  Created on: Aug 21, 2024
 *      Author: DPY
 */
#include "ultrasonic.h"

uint32_t ultrasonic_num = 0;
uint8_t ultrasonic_flag = 0; // 0:没开始测距  1:开始测距 0: Ranging not started 1: Ranging starte

void ultrasonic_init(void)   //使用超声前需在主函数中调用此初始化函数
{
	HAL_TIM_Base_Start_IT(&htim7);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)  //TIM7中断回调函数，用于记录整周期数
{
	if (htim->Instance == TIM7)
	{
		if (ultrasonic_flag) // 开始测距--超声波
		{
			ultrasonic_num++;  //记录整周期
		}
	}
}

float Get_distance(void)         //距离测试函数
{
	float distance = 0, aveg = 0;
	uint16_t tim, count;
	uint8_t i = 0;

	while (i != 5)
	{
		HAL_GPIO_WritePin(TRIG_GPIO_Port, TRIG_Pin, GPIO_PIN_SET);
		Delay_US(20);
		HAL_GPIO_WritePin(TRIG_GPIO_Port, TRIG_Pin, GPIO_PIN_RESET);

		while (HAL_GPIO_ReadPin(ECHO_GPIO_Port, ECHO_Pin) == GPIO_PIN_RESET)
			;
		ultrasonic_flag = 1;

		i += 1;
		while (HAL_GPIO_ReadPin(ECHO_GPIO_Port, ECHO_Pin) == GPIO_PIN_SET)
		{
			count = ultrasonic_num;
			if (count >= 10000)
			{
				ultrasonic_flag = 0;
				ultrasonic_num = 0;
				return 0;
			}
		}

		ultrasonic_flag = 0;
		tim = TIM7->CNT; //非整周期
		distance = (tim + ultrasonic_num * 10) / 58.5;
		aveg = distance + aveg;
		ultrasonic_num = 0;
		HAL_Delay(10);
	}
	distance = aveg / 5;     //测试5次取平均值
	return distance;
}

void Delay_US(uint32_t nus) //us延时函数，使用时调用即可
{
	uint32_t ticks;
	uint32_t told, tnow, tcnt = 0;
	uint32_t reload = SysTick->LOAD;        /* LOAD的值  LOAD value*/
	ticks = nus * 72;                 /* 需要的节拍数  number of beats required*/
	 told = SysTick->VAL;                    /* 刚进入时的计数器值  Counter value when first entered*/
	    while (1)
	    {
	        tnow = SysTick->VAL;
	        if (tnow != told)
	        {
	            if (tnow < told)
	            {
	                tcnt += told - tnow;        /* 这里注意一下SYSTICK是一个递减的计数器就可以了 Just note here that SYSTICK is a decrementing counter. */
	            }
	            else
	            {
	                tcnt += reload - tnow + told;
	            }
	            told = tnow;
	            if (tcnt >= ticks)
	            {
	                break;                      /* 时间超过/等于要延迟的时间,则退出 If the time exceeds/is equal to the time to be delayed, exit */
	            }
	        }
	    }
}
