/*
 * uart.c - UART 外设操作实现
 */

#include "uart.h"

void uart_init(void)
{
    UART_IER = 0;
}

int uart_rx_valid(void)
{
    return (UART_SR & UART_SR_RX_VALID) != 0;
}

int uart_rx_full(void)
{
    return (UART_SR & UART_SR_RX_FULL) != 0;
}

char uart_getchar(void)
{
    /* 等待数据可用 */
    while (!uart_rx_valid());
    return (char)(UART_RX & 0xFF);
}
