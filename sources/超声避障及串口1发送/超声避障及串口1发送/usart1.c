/*
 * usart1.c
 *
 *  Created on: Aug 30, 2024
 *      Author: DPY
 */


#define USART_DEBUG huart1
//uint8_t RxTemp = 0;
#include "usart1.h"

#ifdef __GNUC__
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif

PUTCHAR_PROTOTYPE
{
  HAL_UART_Transmit(&USART_DEBUG, (uint8_t *)&ch, 1, 0xFFFF); // 阻塞方式打印,串口x
  return ch;
}

void usart1_transfer(uint8_t number){
printf("Hello world:%d \r\n", number);
}
