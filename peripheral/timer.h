/*
 * timer.h - 通用定时器外设
 * 提供 64-bit 单调计数器 mtime 和比较器 mtimecmp
 * mtime >= mtimecmp 时触发定时器中断 (MTIP)
 *
 * 寄存器布局:
 *   +0x00: mtime[31:0]   (只读)
 *   +0x04: mtime[63:32]  (只读)
 *   +0x08: mtimecmp[31:0]  (读写, 写后清除 MTIP)
 *   +0x0C: mtimecmp[63:32] (读写, 写后清除 MTIP)
 */

#ifndef TIMER_H
#define TIMER_H

#include "peripheral.h"

#define TIMER_BASE       0x8300
#define TIMER_SIZE       0x10

/* 寄存器偏移 */
#define TIMER_MTIME_LO    0x00
#define TIMER_MTIME_HI    0x04
#define TIMER_MTIMECMP_LO 0x08
#define TIMER_MTIMECMP_HI 0x0C

/* 创建定时器外设实例 */
Peripheral *timer_create(void);

/* 获取定时器值 (调试用) */
uint64_t timer_get_mtime(Peripheral *p);
uint64_t timer_get_mtimecmp(Peripheral *p);

#endif /* TIMER_H */
