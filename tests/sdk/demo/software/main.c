#include "hw.h"
#include "uart.h"
#include "mtimer.h"
#include "software.h"
static uint32_t flag = 0;

static void busy_delay(volatile uint32_t count)
{
    while (count--) { asm volatile ("nop"); }
}

int main(void)
{
    char ch = 0;
    uart_init();
    MyPrintf("software test demo !!!\r\n");

    while (1)
    {
        flag = 0;
        sw_irq_set();
        asm volatile ("csrs mie, %0" :: "r"(1 << 3));
        while (!flag);
        if(uart_rx_valid())
        {
            ch = uart_getchar();
            if(ch == 'q')
            {
                MyPrintf("quit test demo !!!\r\n");
                break;
            }
            else
            {
                MyPrintf("ch = %c\r\n", ch);
            }
        }
    }
    return 0;
}

__attribute__((interrupt)) void VECTOR_SOFT_IRQ(void)
{
    flag = 1;
    MyPrintf("VECTOR_SOFT_IRQ \r\n");
    sw_irq_clear();
}