/*
 * uart.h - UART 外设操作接口
 * TX: 0x8200 (写出发送)
 * RX: 0x8210 (读取接收)
 * SR: 0x8214 (状态: bit0=rx_valid, bit1=rx_full)
 */

#ifndef UART_H__
#define UART_H__

#include <stdint.h>

#define UART_TX_ADDR    0x8200
#define UART_RX_ADDR    0x8210
#define UART_SR_ADDR    0x8214
#define UART_IER_ADDR   0x8218

#define UART_TX    (*(volatile uint32_t *)UART_TX_ADDR)
#define UART_RX    (*(volatile uint32_t *)UART_RX_ADDR)
#define UART_SR    (*(volatile uint32_t *)UART_SR_ADDR)
#define UART_IER   (*(volatile uint32_t *)UART_IER_ADDR)

#define UART_SR_RX_VALID  0x01  /* bit0: rx data available */
#define UART_SR_RX_FULL   0x02  /* bit1: rx buffer full     */
#define UART_IER_RX_EN    0x01  /* bit0: rx interrupt enable */

void uart_init(void);
int  uart_rx_valid(void);
int  uart_rx_full(void);
char uart_getchar(void);

#endif /* UART_H__ */
