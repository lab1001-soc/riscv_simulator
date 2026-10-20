/*
 * software.h - 软件中断外设
 * 通过 MMIO 寄存器设置/清除机器软件中断 (MSIP, mip bit 3)
 *
 * 寄存器布局:
 *   +0x00: SW_IRQ_CTRL — 写1置MSIP, 写0清MSIP; 读返回当前MSIP状态
 */

#ifndef SOFTWARE_H
#define SOFTWARE_H

#include "peripheral.h"

#define SOFTWARE_BASE   0x8400
#define SOFTWARE_SIZE   0x04

/* 寄存器偏移 */
#define SW_IRQ_CTRL      0x00

/* 创建软件中断外设实例 */
Peripheral *software_create(void);

#endif /* SOFTWARE_H */
