/*
 * plic.c - 最小 SiFive PLIC (Platform-Level Interrupt Controller)
 *
 * 内存布局（与内核 drivers/irqchip/irq-sifive-plic.c 匹配）：
 *   priority[src]     0x0C000000 + src*4        读写
 *   pending           0x0C001000 + word*4       只读（位图）
 *   enable[ctx]       0x0C002000 + ctx*0x80     读写（位图）
 *   threshold[ctx]    0x0C200000 + ctx*0x1000   读写
 *   claim/complete    threshold + 4             读=取号+清pending；写=complete
 *
 * 单 S-mode context（DTB interrupts-extended 只声明 <cpu_intc 9>）。
 * 输出线只接 mip/sip.SEIP（见 plic_update_output 注释：不能走 MEIP+委托）。
 *
 * 设备中断线为电平语义：claim 清 pending 后若线仍为高会立即重新置位，
 * 内核 plic_handle_irq 的 while(readl(claim)) 循环在设备 deassert 前持续分发，
 * 与真实硬件行为一致。
 */

#include "plic.h"
#include "cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ENABLE_WORD(ctx, word)  (0x2000 + (ctx) * 0x80 + (word) * 4)
#define CTX_BASE(ctx)           (0x200000 + (ctx) * 0x1000)

typedef struct {
    uint32_t priority[PLIC_NDEV + 1];                       /* [0] 保留 */
    uint32_t enable[PLIC_NCTX][(PLIC_NDEV + 31) / 32];      /* 位图 */
    uint32_t threshold[PLIC_NCTX];
    uint8_t  asserted[PLIC_NDEV + 1];   /* 设备中断线电平 */
    uint8_t  pending[PLIC_NDEV + 1];    /* PLIC 内部锁存 */
} PLIC_Priv;

/* ---------- 输出重估：任一源满足条件则拉高 hart 的 S-ext 引脚 ----------
 * PLIC 输出线接 cpu-intc 的 S-mode external（DTB interrupts-extended=<9>），
 * 即置 mip.SEIP(bit9)；S 态经 sip 视图可见（模拟器 mip/sip 为独立变量，
 * 需同步写）。不能走 MEIP+委托：mideleg 未委托 MEIP 时 S 态永远看不到。 */
static void plic_update_output(Peripheral *p, struct CPU *cpu) {
    PLIC_Priv *u = (PLIC_Priv *)p->priv;
    int src, out = 0;

    for (src = 1; src <= PLIC_NDEV; src++) {
        if (u->pending[src] &&
            (u->enable[0][src / 32] & (1u << (src % 32))) &&
            u->priority[src] > u->threshold[0]) {
            out = 1;
            break;
        }
    }

    if (out) {
        cpu->mip |= MIP_SEIP;
        cpu->sip |= SIP_SEIP;
    } else {
        cpu->mip &= ~MIP_SEIP;
        cpu->sip &= ~SIP_SEIP;
    }
}

/* ---------- match: PLIC 地址范围 ---------- */
static int plic_match(Peripheral *p, uint32_t addr) {
    (void)p;
    return (addr >= PLIC_BASE && addr < PLIC_BASE + PLIC_SIZE);
}

/* ---------- read ---------- */
static uint32_t plic_read(Peripheral *p, uint32_t addr, int size,
                          struct CPU *cpu) {
    PLIC_Priv *u = (PLIC_Priv *)p->priv;
    uint32_t off = addr - PLIC_BASE;

    if (off < 0x1000) {
        /* priority 区（source 0 保留） */
        int src = off / 4;
        return (src >= 1 && src <= PLIC_NDEV) ? u->priority[src] : 0;
    } else if (off >= 0x1000 && off < 0x2000) {
        /* pending 位图（只读） */
        int word = (off - 0x1000) / 4;
        uint32_t v = 0;
        int i;
        for (i = word * 32 + 1; i <= PLIC_NDEV && i < (word + 1) * 32; i++)
            if (u->pending[i])
                v |= 1u << (i % 32);
        return v;
    } else if (off >= 0x2000 && off < 0x200000) {
        /* enable 位图 */
        int word = (off - 0x2000) / 4;
        if (word < PLIC_NCTX * (int)(sizeof(u->enable[0]) / 4))
            return ((uint32_t *)u->enable)[word];
        return 0;
    } else if (off == CTX_BASE(0)) {
        return u->threshold[0];
    } else if (off == CTX_BASE(0) + 4) {
        /* claim：选最高优先级（同级取最小编号）的 pending 源 */
        int best = 0, src;
        for (src = 1; src <= PLIC_NDEV; src++) {
            if (u->pending[src] &&
                (u->enable[0][src / 32] & (1u << (src % 32))) &&
                u->priority[src] > u->threshold[0]) {
                if (best == 0 || u->priority[src] > u->priority[best])
                    best = src;
            }
        }
        if (best) {
            u->pending[best] = 0;
            /* 电平源重采样：线仍为高则立即重新 pending（真硬件语义） */
            if (u->asserted[best])
                u->pending[best] = 1;
            plic_update_output(p, cpu);
        }
        return (uint32_t)best;
    }
    (void)size;
    return 0;
}

/* ---------- write ---------- */
static void plic_write(Peripheral *p, uint32_t addr, uint32_t value,
                       int size, struct CPU *cpu) {
    PLIC_Priv *u = (PLIC_Priv *)p->priv;
    uint32_t off = addr - PLIC_BASE;

    if (off < 0x1000) {
        int src = off / 4;
        if (src >= 1 && src <= PLIC_NDEV)
            u->priority[src] = value & 0x7;
    } else if (off >= 0x2000 && off < 0x200000) {
        int word = (off - 0x2000) / 4;
        if (word < PLIC_NCTX * (int)(sizeof(u->enable[0]) / 4))
            ((uint32_t *)u->enable)[word] = value;
    } else if (off == CTX_BASE(0)) {
        u->threshold[0] = value & 0x7;
    } else if (off == CTX_BASE(0) + 4) {
        /* complete：清 pending（fasteoi eoi 写回 claim 值） */
        if (value >= 1 && value <= PLIC_NDEV)
            u->pending[value] = 0;
    }
    /* priority/enable/threshold 变化都会影响输出 */
    plic_update_output(p, cpu);
    (void)size;
}

/* ---------- tick ---------- */
static void plic_tick(Peripheral *p, struct CPU *cpu) {
    (void)p;
    (void)cpu;
}

/* ---------- 公共 API ---------- */

Peripheral *plic_create(void) {
    Peripheral *p = calloc(1, sizeof(Peripheral));
    PLIC_Priv  *u = calloc(1, sizeof(PLIC_Priv));

    p->name  = "plic";
    p->match = plic_match;
    p->read  = plic_read;
    p->write = plic_write;
    p->tick  = plic_tick;
    p->priv  = u;

    return p;
}

void plic_set_source(Peripheral *p, struct CPU *cpu, int src, int level) {
    PLIC_Priv *u = (PLIC_Priv *)p->priv;

    if (src < 1 || src > PLIC_NDEV)
        return;

    u->asserted[src] = level ? 1 : 0;
    if (level)
        u->pending[src] = 1;
    else
        u->pending[src] = 0;   /* 电平：线撤销即中断条件消失 */

    plic_update_output(p, cpu);
}
