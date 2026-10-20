/*
 * uart.h - 虚拟 UART 外设
 * 通过宿主机 stdin/stdout 进行串口通信
 * TX: 0x8200 (写出发送)
 * RX: 0x8210 (读取接收)
 */

#ifndef UART_H
#define UART_H

#include "peripheral.h"

/* 地址常量 */
#define UART_BASE      0x8200
#define UART_TX_ADDR   (UART_BASE + 0x00)   /* TX: 写出发送 */
#define UART_RX_ADDR   (UART_BASE + 0x10)   /* RX: 读取接收 */
#define UART_SR_ADDR   (UART_BASE + 0x14)   /* SR: 状态 (bit0=rx_valid, bit1=rx_full) */
#define UART_IER_ADDR  (UART_BASE + 0x18)   /* IER: 中断使能 (bit0=rx_irq_en) */
#define UART_RX_CNT_ADDR (UART_BASE + 0x1C) /* RX_CNT: RX缓冲区可用字节数 */

/* 创建 UART 外设实例 */
Peripheral *uart_create(void);

/* 从宿主机向 UART 注入字符 (由键盘处理函数调用，触发外部中断) */
void uart_inject_char(Peripheral *p, uint8_t ch, struct CPU *cpu);

#endif /* UART_H */
