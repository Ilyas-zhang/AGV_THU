#include <usart2.h>

//char buf_msg[20] = {'\0'};
//uint8_t g_new_flag = 0;
//uint8_t g_index = 0;

// 函数功能:打开串口2接收中断功能
// 传入函数:无
// Function function: Enable serial port 2 to receive interrupt function
// Incoming function: None
void USART2_UART_Init(void)
{

	LL_USART_EnableIT_RXNE(USART2); // Start receiving interrupt 启动接收中断
}

/**
 * @brief This function handles USART2 global interrupt.
 */
void USART2_IRQHandler(void)
{
	uint8_t rx2_temp;
	if (LL_USART_IsEnabledIT_RXNE(USART2)) // Determine if there is any interruption information 判断是否有中断信息
	{
		LL_USART_ClearFlag_RXNE(USART2); //clear interrupt 清除中断
		rx2_temp = LL_USART_ReceiveData8(USART2); // Read information and clear interrupts 读取信息并清除中断
		//Deal_K210(rx2_temp);					  // Processing data sent by K210 处理k210送来的数据
		USART2_DataByte(rx2_temp);//send data 发送数据
		//printf("%d, \r\n",rx2_temp);
		if (rx2_temp == '1')
		{
			pwm_motor1_forward(3000);
			Delay_ms(1000);
			pwm_motor_stop();
		}
		if (rx2_temp == '2')
		{
			pwm_motor2_forward(3000);
			Delay_ms(1000);
			pwm_motor_stop();
		}
		if (rx2_temp == '3')
		{
			pwm_motor3_forward(3000);
			Delay_ms(1000);
			pwm_motor_stop();
		}
		if (rx2_temp == '4')
		{
			pwm_motor4_forward(3000);
			Delay_ms(1000);
			pwm_motor_stop();
		}
		//memset(buf_msg, 0, sizeof(buf_msg));
	}
}


// Send a Byte 发送一个字节
// data_byte:Sent data 发送的数据
void USART2_DataByte(uint8_t data_byte)
{
	while (!LL_USART_IsActiveFlag_TXE(USART2))
	{
	};
	LL_USART_TransmitData8(USART2, data_byte);
}

// Set to send a string 设置发送一个字符串
// data_str :The first address of the data 数据的首地址
// datasize :The length of data 数据的长度

void USART2_DataString(uint8_t *data_str, uint16_t datasize)
{
	for (uint8_t len = 0; len < datasize; len++)
	{
		USART2_DataByte(*(data_str + len));
	}
}

void Delay_ms(uint16_t time)
{
    uint16_t i=0;
    while(time--)
    {
        i=3127;
        while(i--);
    }
}

//void Deal_K210(uint8_t recv_msg)
//{
//	if (recv_msg == '$' && g_new_flag == 0)
//	{
//		g_new_flag = 1;
//		memset(buf_msg, 0, sizeof(buf_msg)); // Clear old data 清除旧数据
//		return;
//	}
//	if(g_new_flag == 1)
//	{
//		if (recv_msg == '#')
//		{
//			g_new_flag = 0;
//			g_index = 0;
//			//Change_RGB(); // New data received completed 新数据接收完毕
//			memset(buf_msg, 0, sizeof(buf_msg)); // Clear old data 清除旧数据
//		}
//
//		else if (g_new_flag == 1 && recv_msg != '$')
//		{
//			buf_msg[g_index++] = recv_msg;
//
//			if(g_index > 20) //数组溢出 Array overflow
//			{
//				g_index = 0;
//				g_new_flag = 0;
//				memset(buf_msg, 0, sizeof(buf_msg)); // Clear old data 清除旧数据
//			}
//		}
//	}
//}

//void BSP_Init(void)
//{
//	char senf_buff[10] = {'\0'};
//	// 这个库初始化完成会有0xff的数据
//	// After the initialization of this library is completed, there will be 0xff data sent
//	//USART2_UART_Init();
//	HAL_Delay(10);
//	//strcpy(senf_buff, "$close#");
//	//USART2_DataString((uint8_t *)senf_buff, sizeof(senf_buff)); // 关闭k210-RGB灯
//}


