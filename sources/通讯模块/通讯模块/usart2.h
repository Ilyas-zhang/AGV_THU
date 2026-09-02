#ifndef __BSP_USART2_H__
#define __BSP_USART2_H__

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usart.h"
#include "pwm_motor.h"

void USART2_UART_Init(void);
void USART2_DataByte(uint8_t data_byte);
void USART2_DataString(uint8_t *data_str, uint16_t datasize);
void Deal_K210(uint8_t recv_msg);
void Delay_ms(uint16_t time);
//void BSP_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* __USART_H__ */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
