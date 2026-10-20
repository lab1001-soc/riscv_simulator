/*
 * software.c - 软件中断外设实现
 * 写 0x8400 bit0=1 置位 MIP_MSIP, bit0=0 清除 MIP_MSIP
 * 读 0x8400 返回当前 MSIP 状态
 */

#include "software.h"
#include "cpu.h"
#include <stdio.h>
#include <stdlib.h>

/* ---------- match: 判断地址是否属于软件中断外设 ---------- */
static int software_match(Peripheral *p, uint32_t addr) {
    (void)p;
    return (addr >= SOFTWARE_BASE && addr < SOFTWARE_BASE + SOFTWARE_SIZE);
}

/* ---------- read: 读当前 MSIP 状态 ---------- */
static uint32_t software_read(Peripheral *p, uint32_t addr, int size,
                              struct CPU *cpu) {
    (void)p;
    (void)addr;
    (void)size;
    return (cpu->mip & MIP_MSIP) ? 1 : 0;
}

/* ---------- write: 设置/清除 MSIP ---------- */
static void software_write(Peripheral *p, uint32_t addr, uint32_t value,
                           int size, struct CPU *cpu) {
    (void)p;
    (void)addr;
    (void)size;

    if (value & 1) {
        cpu->mip |= MIP_MSIP;    /* 置位软件中断 */
        /* 如果 MSIP 被委托给 S-mode，同步设置 SSIP */
        if (cpu->mideleg & MIP_MSIP)
            cpu->sip |= SIP_SSIP;
    } else {
        cpu->mip &= ~MIP_MSIP;   /* 清除软件中断 */
        if (cpu->mideleg & MIP_MSIP)
            cpu->sip &= ~SIP_SSIP;
    }
}

/* ---------- tick: 软件中断不需要 per-cycle 操作 ---------- */
static void software_tick(Peripheral *p, struct CPU *cpu) {
    (void)p;
    (void)cpu;
}

/* ---------- 公共 API ---------- */

Peripheral *software_create(void) {
    Peripheral *p = calloc(1, sizeof(Peripheral));

    p->name  = "software";
    p->match = software_match;
    p->read  = software_read;
    p->write = software_write;
    p->tick  = software_tick;
    p->priv  = NULL;

    return p;
}
