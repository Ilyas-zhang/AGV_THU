#ifndef __K210_COMM_CONFIG_H
#define __K210_COMM_CONFIG_H

/*
 * K210 通讯参数配置
 */

/* ---- 帧缓冲区 ---- */
#define K210_RX_BUF_SIZE         64      /* 接收帧最大长度（不含 $ #） */

/* ---- 发送超时 ---- */
#define K210_TX_BYTE_TIMEOUT     100     /* 单字节发送超时 ms */
#define K210_TX_STRING_TIMEOUT   1000    /* 字符串发送超时 ms */

#endif /* __K210_COMM_CONFIG_H */
