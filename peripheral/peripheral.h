/*
 * peripheral.h - 统一外设接口
 * 所有 MMIO 外设 (UART, CLINT, PLIC 等) 实现此接口
 */

#ifndef PERIPHERAL_H
#define PERIPHERAL_H

#include <stdint.h>

struct CPU;  /* 前向声明 */

typedef struct Peripheral {
    const char *name;   /* 调试用名字 */

    /* 判断地址是否属于本外设，返回 1=命中 0=未命中 */
    int      (*match)(struct Peripheral *p, uint32_t addr);

    /* 读外设寄存器 (addr 已通过 match 确认属于此外设) */
    uint32_t (*read )(struct Peripheral *p, uint32_t addr, int size,
                      struct CPU *cpu);

    /* 写外设寄存器 (addr 已通过 match 确认属于此外设) */
    void     (*write)(struct Peripheral *p, uint32_t addr, uint32_t value,
                      int size, struct CPU *cpu);

    /* 每 CPU 周期调用：驱动定时器、检查中断产生条件 */
    void     (*tick )(struct Peripheral *p, struct CPU *cpu);

    void *priv;  /* 外设私有数据 */
} Peripheral;

#endif /* PERIPHERAL_H */
