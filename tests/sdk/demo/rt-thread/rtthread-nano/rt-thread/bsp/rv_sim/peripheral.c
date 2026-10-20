#include "hw.h"
#include "uart.h"
#include "mtimer.h"
#include <rtthread.h>
#include "riscv_encoding.h"


#define _UARTA  0
void print_init(char uart, char tx,char rx)
{
    uart_init();
}

static void _uart_tx_byte(char uart,byte c)
{
	*(volatile byte *)0x8200 = c;
}

char get_uart_byte(char uart)
{
    char ret;
    /* Note: the initial value of ch must < 0 */
    if(uart_rx_valid() == 0)
        return -1;

    return uart_getchar();
}

void printf_send(char uart,uint8_t *buf, int len)
{
    for (int i = 0; i < len; i++)
    {
        _uart_tx_byte(uart,buf[i]);
    }
}

static int chip_init(void)
{
    asm("la t0,_sp");
    asm("csrw mscratch,t0");
    return 0;
}
INIT_BOARD_EXPORT(chip_init);

static volatile uint64_t period = 0;
void systick_config(uint64_t p)
{
    period = p;
    timer_set_cmp(timer_get_mtime() + period);
    timer_enable_intr();
}

static uint64_t last_mtime = 0;
__attribute__((interrupt)) void  SYSTICK_IRQHandler(void)
{
    uint64_t mtime = timer_get_mtime();
    uint64_t mtimer_next = mtime + period;
    timer_set_cmp(mtimer_next);
    // rt_kprintf("SYSTICK_IRQ, mtime: %u, interval: %u,period: %u\n",(uint32_t)mtime,(uint32_t)(mtime-last_mtime),period);

    last_mtime = mtime;
    rt_interrupt_enter();

    rt_tick_increase();

    rt_interrupt_leave();

}