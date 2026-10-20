/*
 * menu demo - 串口菜单 + 定时器秒表 + 软件中断
 * 第 21 篇《搭建芯片 SDK》的实战示例:
 *   串口打印菜单 → 按键选择 → mtime 读取 / 秒表(定时器中断) / 软件中断自测
 *
 * 中断说明:startup.S 已开 mstatus.MIE 并把 mtvec 设为 Vectored 模式,
 * 槽 3(软件中断)/槽 7(定时器中断)直跳本文件的 ISR,C 函数须带
 * __attribute__((interrupt)) 由编译器自动保存现场并以 mret 返回。
 */

#include "hw.h"
#include "uart.h"
#include "mtimer.h"
#include "software.h"

#define MIE_MSIE        (1u << 3)     /* mie: 软件中断使能 */
#define TICKS_PER_SEC   1000000u      /* timebase 1MHz:1s = 1,000,000 mtime */

static volatile uint32_t sw_count;     /* 软件中断触发次数 */
static volatile uint32_t tick_count;   /* 秒表滴答数 */
static volatile uint32_t tick_target;  /* 目标滴答数 */

static void print_menu(void)
{
    MyPrintf("\r\n=== menu test demo ===\r\n");
    MyPrintf("[1] show mtime\r\n");
    MyPrintf("[2] stopwatch (5 x 1s)\r\n");
    MyPrintf("[3] software irq x3\r\n");
    MyPrintf("[q] quit\r\n");
}

int main(void)
{
    char ch;

    uart_init();
    timer_init();
    print_menu();

    while (1)
    {
        if (!uart_rx_valid())
            continue;
        ch = uart_getchar();

        switch (ch)
        {
        case '1':
            MyPrintf("mtime = %u\r\n", (uint32_t)timer_get_mtime());
            break;

        case '2':
            tick_count = 0;
            tick_target = 5;
            timer_set_cmp(timer_get_mtime() + TICKS_PER_SEC);
            timer_enable_intr();
            while (tick_count < tick_target)
                ;   /* 等 SYSTICK_IRQHandler 打满 5 拍 */
            timer_disable_intr();
            MyPrintf("stopwatch done, %u ticks\r\n", (uint32_t)tick_count);
            break;

        case '3':
            for (uint32_t i = 1; i <= 3; i++)
            {
                sw_count = 0;
                asm volatile ("csrs mie, %0" :: "r"(MIE_MSIE));
                sw_irq_set();
                while (!sw_count)
                    ;   /* 等 VECTOR_SOFT_IRQ 置位 */
                MyPrintf("soft irq #%u ok\r\n", i);
            }
            break;

        case 'q':
            MyPrintf("quit menu demo !!!\r\n");
            return 0;

        default:
            break;
        }
        print_menu();
    }
    return 0;
}

/* ---- 定时器中断:槽 7 直跳,打印滴答并重装 mtimecmp(写 cmp 顺带清 MTIP) ---- */
__attribute__((interrupt)) void SYSTICK_IRQHandler(void)
{
    uint64_t mtime = timer_get_mtime();
    tick_count++;
    MyPrintf("  tick %u: mtime=%u\r\n", (uint32_t)tick_count, (uint32_t)mtime);
    timer_set_cmp(mtime + TICKS_PER_SEC);
}

/* ---- 软件中断:槽 3 直跳,清 MSIP(写 0)避免重复触发 ---- */
__attribute__((interrupt)) void VECTOR_SOFT_IRQ(void)
{
    sw_count++;
    sw_irq_clear();
}
