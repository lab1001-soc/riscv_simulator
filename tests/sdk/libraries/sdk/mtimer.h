/*
 * timer.h - Timer peripheral operation interface
 *
 * Register layout (simulator timer at 0x8300):
 *   +0x00: MTIME[31:0]    (read-only)
 *   +0x04: MTIME[63:32]   (read-only)
 *   +0x08: MTIMECMP[31:0] (read/write, writing clears MTIP)
 *   +0x0C: MTIMECMP[63:32](read/write, writing clears MTIP)
 *
 * mtime increments every CPU cycle. When mtime >= mtimecmp,
 * machine timer interrupt (MTIP, bit 7 of mip) is asserted.
 */

#ifndef TIMER_H__
#define TIMER_H__

#include <stdint.h>

#define TIMER_BASE          0x8300
#define TIMER_MTIME_LO_ADDR  (TIMER_BASE + 0x00)
#define TIMER_MTIME_HI_ADDR  (TIMER_BASE + 0x04)
#define TIMER_MTIMECMP_LO_ADDR (TIMER_BASE + 0x08)
#define TIMER_MTIMECMP_HI_ADDR (TIMER_BASE + 0x0C)

#define TIMER_MTIME_LO    (*(volatile uint32_t *)TIMER_MTIME_LO_ADDR)
#define TIMER_MTIME_HI    (*(volatile uint32_t *)TIMER_MTIME_HI_ADDR)
#define TIMER_MTIMECMP_LO (*(volatile uint32_t *)TIMER_MTIMECMP_LO_ADDR)
#define TIMER_MTIMECMP_HI (*(volatile uint32_t *)TIMER_MTIMECMP_HI_ADDR)

void     timer_init(void);
uint64_t timer_get_mtime(void);
void     timer_set_cmp(uint64_t cmp);
uint64_t timer_get_cmp(void);
void     timer_enable_intr(void);
void     timer_disable_intr(void);

#endif /* TIMER_H__ */
