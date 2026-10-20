/*
 * timer.c - 通用定时器外设实现
 * mtime 每 CPU 周期递增 1
 * mtime >= mtimecmp 时置 cpu->mip.MTIP
 */

#include "timer.h"
#include "cpu.h"
#include <stdio.h>
#include <stdlib.h>

/* 定时器私有数据 */
typedef struct {
    uint64_t mtime;
    uint64_t mtimecmp;
    uint64_t mtime_latched;    /* 读 MTIME_HI 时锁存完整 mtime */
    uint64_t mtimecmp_latched; /* 读 MTIMECMP_HI 时锁存完整 mtimecmp */
} Timer_Priv;

/* ---------- match: 判断地址是否属于定时器 ---------- */
static int timer_match(Peripheral *p, uint32_t addr) {
    (void)p;
    return (addr >= TIMER_BASE && addr < TIMER_BASE + TIMER_SIZE);
}

/* ---------- read: 读定时器寄存器 ---------- */
static uint32_t timer_read(Peripheral *p, uint32_t addr, int size,
                           struct CPU *cpu) {
    Timer_Priv *t = (Timer_Priv *)p->priv;
    uint32_t offset = addr - TIMER_BASE;
    uint32_t ret = 0;

    switch (offset) {
        case TIMER_MTIME_HI:
            t->mtime_latched = t->mtime;   /* 读高地址锁存完整 mtime */
            ret = (uint32_t)(t->mtime >> 32);
            break;
        case TIMER_MTIME_LO:
            ret = (uint32_t)t->mtime_latched;  /* 返回锁存的低 32 位 */
            break;
        case TIMER_MTIMECMP_HI:
            t->mtimecmp_latched = t->mtimecmp;  /* 读高地址锁存完整 mtimecmp */
            ret = (uint32_t)(t->mtimecmp >> 32);
            break;
        case TIMER_MTIMECMP_LO:
            ret = (uint32_t)t->mtimecmp_latched;  /* 返回锁存的低 32 位 */
            break;
        default:
            ret = 0;
            break;
    }

    (void)size;
    (void)cpu;
    return ret;
}

/* ---------- write: 写定时器寄存器 ---------- */
static void timer_write(Peripheral *p, uint32_t addr, uint32_t value,
                        int size, struct CPU *cpu) {
    Timer_Priv *t = (Timer_Priv *)p->priv;
    uint32_t offset = addr - TIMER_BASE;

    switch (offset) {
        case TIMER_MTIMECMP_LO:
            t->mtimecmp = (t->mtimecmp & ~0xFFFFFFFFull) | value;
            cpu->mip &= ~MIP_MTIP;  /* 写 mtimecmp 清除旧的定时器中断 */
            /* STIP 委托位是 mideleg bit5 (cause 5)，不是 MIP_MTIP(bit7) */
            if (cpu->mideleg & SIP_STIP)
                cpu->sip &= ~SIP_STIP;
            break;
        case TIMER_MTIMECMP_HI:
            t->mtimecmp = (t->mtimecmp & 0xFFFFFFFFull)
                        | ((uint64_t)value << 32);
            cpu->mip &= ~MIP_MTIP;
            if (cpu->mideleg & SIP_STIP)
                cpu->sip &= ~SIP_STIP;
            break;
        default:
            break;  /* mtime 只读，忽略写入 */
    }

    (void)size;
}

/* ---------- tick: 每周期递增 mtime，检查定时器中断 ---------- */
static void timer_tick(Peripheral *p, struct CPU *cpu) {
    Timer_Priv *t = (Timer_Priv *)p->priv;

    t->mtime++;
    if (t->mtime >= t->mtimecmp) {
        cpu->mip |= MIP_MTIP;
        /* 定时器中断委托给 S-mode 的条件是 mideleg bit5 (STIP cause 5) */
        if (cpu->mideleg & SIP_STIP)
            cpu->sip |= SIP_STIP;
    }
}

/* ---------- 公共 API ---------- */

Peripheral *timer_create(void) {
    Peripheral *p = calloc(1, sizeof(Peripheral));
    Timer_Priv *t = calloc(1, sizeof(Timer_Priv));

    t->mtime    = 0;
    t->mtimecmp = ~0ull;  /* 初始为最大值，不触发中断 */
    t->mtime_latched = 0;
    t->mtimecmp_latched = ~0ull;

    p->name  = "timer";
    p->match = timer_match;
    p->read  = timer_read;
    p->write = timer_write;
    p->tick  = timer_tick;
    p->priv  = t;

    return p;
}

uint64_t timer_get_mtime(Peripheral *p) {
    return ((Timer_Priv *)p->priv)->mtime;
}

uint64_t timer_get_mtimecmp(Peripheral *p) {
    return ((Timer_Priv *)p->priv)->mtimecmp;
}
