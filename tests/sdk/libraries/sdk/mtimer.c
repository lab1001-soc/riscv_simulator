/*
 * timer.c - Timer peripheral operation implementation
 */

#include "mtimer.h"

void timer_init(void)
{
    /* Set mtimecmp to max value, no timer interrupt */
    TIMER_MTIMECMP_LO = 0xFFFFFFFF;
    TIMER_MTIMECMP_HI = 0xFFFFFFFF;
}

uint64_t timer_get_mtime(void)
{
    uint64_t hi, lo;

    /* Read hi-lo-hi to handle 32-bit overflow across reads */
    hi = TIMER_MTIME_HI;
    lo = TIMER_MTIME_LO;
    if (hi != TIMER_MTIME_HI) {
        hi = TIMER_MTIME_HI;
        lo = TIMER_MTIME_LO;
    }
    return (hi << 32) | lo;
}

void timer_set_cmp(uint64_t cmp)
{
    /* Write high first to avoid spurious match */
    TIMER_MTIMECMP_HI = (uint32_t)(cmp >> 32);
    TIMER_MTIMECMP_LO = (uint32_t)(cmp & 0xFFFFFFFF);
}

uint64_t timer_get_cmp(void)
{
    uint64_t hi, lo;

    hi = TIMER_MTIMECMP_HI;
    lo = TIMER_MTIMECMP_LO;
    return (hi << 32) | lo;
}

void timer_enable_intr(void)
{
    /* Set MTIE (Machine Timer Interrupt Enable) bit 7 in mie */
    asm volatile ("csrs mie, %0" :: "r"(1 << 7));
}

void timer_disable_intr(void)
{
    /* Clear MTIE bit 7 in mie */
    asm volatile ("csrc mie, %0" :: "r"(1 << 7));
}
