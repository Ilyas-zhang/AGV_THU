# Untitled - By: DPY - 周五 8月 30 2024

#导入标准库
from machine import UART
from board import board_info
from fpioa_manager import fm
from modules import ybrgb
import time
import utime

#在使用uart前，需要使用fm来对串口引脚进行映射和管理
fm.register(8, fm.fpioa.UART1_TX, force=True)
fm.register(6, fm.fpioa.UART1_RX, force=True)

#初始化函数串口收发函数
uart_A = UART(UART.UART1, 115200, 8, 0, 1, timeout=1000, read_buf_len=4096)

while True:

        uart_A.write("$"+"hello world"+"#")   #使用串口发送数据
        utime.sleep_ms(500)             #延时

uart_A.deinit()                         #注销UART硬件
del uart_A
