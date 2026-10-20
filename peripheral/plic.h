#ifndef PLIC_H
#define PLIC_H

#include "peripheral.h"

/* PLIC 内存映射（对齐 QEMU virt：基址 0x0C000000，单 S-mode context） */
#define PLIC_BASE       0x0C000000u
#define PLIC_SIZE       0x04000000u
#define PLIC_NDEV       53          /* 中断源数（source 0 保留不存在） */
#define PLIC_NCTX       1           /* 只有 S-mode context 0 */
#define PLIC_UART_SRC   10          /* UART 中断线接 source 10（QEMU virt 同号） */

/* 外设索引（cpu->peripherals[]）：0=uart 1=timer 2=software 3=plic */
#define PERIPH_PLIC_IDX 3

Peripheral *plic_create(void);

/* 设备中断线电平设置（电平触发）：
 * level=1 拉高（置 pending 并重估输出），level=0 拉低（清 pending） */
void plic_set_source(Peripheral *p, struct CPU *cpu, int src, int level);

#endif /* PLIC_H */
