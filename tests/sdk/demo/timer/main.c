#include "hw.h"
#include "uart.h"
#include "mtimer.h"

static void busy_delay(volatile uint32_t count)
{
    while (count--) { asm volatile ("nop"); }
}

int main(void)
{
    uint64_t start, now;

    uart_init();
    timer_init();
    MyPrintf("timer test demo !!\r\n\r\n");

    /* ---- Test 1: read mtime and check it advances ---- */
    MyPrintf("[1] mtime read / advance\r\n");
    start = timer_get_mtime();
    MyPrintf("    initial mtime = %u\r\n", (uint32_t)start);

    busy_delay(100000);
    now = timer_get_mtime();
    MyPrintf("    after delay   = %u (delta=%u)\r\n",
             (uint32_t)now, (uint32_t)(now - start));

    /* ---- Test 2: set compare and poll until reached ---- */
    MyPrintf("\r\n[2] set compare + poll wait\r\n");
    start = timer_get_mtime();
    timer_set_cmp(start + 200000);
    MyPrintf("    cmp  set to %u, mtime=%u\r\n",
             (uint32_t)(start + 200000), (uint32_t)start);

    while (timer_get_mtime() < timer_get_cmp());
    now = timer_get_mtime();
    MyPrintf("    reached cmp at mtime=%u (delta=%u)\r\n",
             (uint32_t)now, (uint32_t)(now - start));

    /* ---- Test 3: periodic timer, 3 intervals ---- */
    MyPrintf("\r\n[3] periodic timer (3 x ~150k cycles)\r\n");
    start = timer_get_mtime();
    for (int i = 1; i <= 3; i++) {
        timer_set_cmp(timer_get_mtime() + 150000);
        while (timer_get_mtime() < timer_get_cmp());
        now = timer_get_mtime();
        MyPrintf("    round %d: mtime=%u (total elapsed=%u)\r\n",
                 i, (uint32_t)now, (uint32_t)(now - start));
    }

    MyPrintf("\r\ntimer test done!\r\n");

    MyPrintf("\r\n[4] timer irq test\r\n");
    timer_set_cmp(timer_get_mtime() + 150000);
    timer_enable_intr();
    while (1);
    return 0;
}

static uint64_t last = 0;
__attribute__((interrupt)) void SYSTICK_IRQHandler(void)
{
    uint64_t mtime = timer_get_mtime();
    MyPrintf("SYSTICK_IRQHandler mtime:%u, delta:%u\r\n", (uint32_t)mtime, mtime - last);
    timer_set_cmp(mtime + 15000000);
    last = mtime;
}