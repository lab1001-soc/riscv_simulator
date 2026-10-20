/*
 * uart.c - 虚拟 UART 外设实现（16550 兼容 + 自定义寄存器）
 * TX: 写 0x8200 → putchar() 输出到宿主终端
 * RX: 读 0x8210 → 返回接收缓冲区的字符
 * SR: 读 0x8214 → 状态 (bit0=rx_valid, bit1=rx_full, bit5/6=THR空)
 * 中断线经 PLIC source 10 汇聚（RX 数据就绪 ERBFI / 发送空 THRE ETBEI）
 */

#include "uart.h"
#include "cpu.h"
#include "plic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RX_BUF_SIZE  256

/* UART 私有数据 */
typedef struct {
    uint8_t rx_buf[RX_BUF_SIZE];  /* 接收环形缓冲区 */
    uint16_t rx_head;             /* 写指针 (uart_inject_char) */
    uint16_t rx_tail;             /* 读指针 (cpu load RX) */
    uint8_t  ier;                 /* 16550 IER: bit0=ERBFI(RX), bit1=ETBEI(THRE) */
    uint8_t  lcr;                 /* 16550 LCR (bit7=DLAB 波特率编程模式) */
    uint8_t  dll, dlm;            /* 波特率分频锁存 */
} UART_Priv;

/* ---------- 缓冲区辅助 ---------- */
static int rx_has_data(UART_Priv *u)   { return u->rx_head != u->rx_tail; }
static int rx_is_full(UART_Priv *u)    { return ((u->rx_head + 1) % RX_BUF_SIZE) == u->rx_tail; }

/* ---------- 更新中断状态 (由外设内部调用) ----------
 * 中断线经 PLIC source 10 汇聚到 hart 外部中断。
 * 电平 = RX 数据就绪(ERBFI) 或 THRE 发送空(ETBEI)。
 * THRE 恒为真（模拟器写 THR 即完成发送），8250 驱动只在有数据待发时
 * 开 ETBEI、发完即关，不会形成持续中断 */
static void uart_update_irq(Peripheral *p, struct CPU *cpu) {
    UART_Priv *u = (UART_Priv *)p->priv;
    int level = ((rx_has_data(u) && (u->ier & 1)) || (u->ier & 2)) ? 1 : 0;

    if (cpu && cpu->num_peripherals > PERIPH_PLIC_IDX)
        plic_set_source(cpu->peripherals[PERIPH_PLIC_IDX], cpu,
                        PLIC_UART_SRC, level);
}

/* ---------- match: 判断地址是否属于 UART ---------- */
static int uart_match(Peripheral *p, uint32_t addr) {
    (void)p;
    return (addr == UART_TX_ADDR  || addr == UART_RX_ADDR ||
            addr == UART_SR_ADDR  || addr == UART_IER_ADDR ||
            addr == UART_RX_CNT_ADDR ||
            addr == UART_TX_ADDR + 0x4 ||   /* 16550 IER/DLM (reg1) */
            addr == UART_TX_ADDR + 0x8 ||   /* 16550 IIR/FCR (reg2) */
            addr == UART_TX_ADDR + 0xC);    /* 16550 LCR (reg3) */
}

/* ---------- read: 读 UART 寄存器 ---------- */
static uint32_t uart_read(Peripheral *p, uint32_t addr, int size,
                          struct CPU *cpu) {
    UART_Priv *u = (UART_Priv *)p->priv;
    uint32_t ret = 0;

    if (addr == UART_RX_ADDR || addr == UART_TX_ADDR) {
        /* 0x8210 自定义 RX；0x8200 兼容 16550 reg0：
           DLAB=1 时为 DLL 波特率锁存，否则 RBR 接收数据 */
        if (u->lcr & 0x80) {
            ret = (addr == UART_TX_ADDR) ? u->dll : 0;
        } else if (rx_has_data(u)) {
            ret = u->rx_buf[u->rx_tail];
            u->rx_tail = (u->rx_tail + 1) % RX_BUF_SIZE;
            uart_update_irq(p, cpu);
        }
    } else if (addr == UART_TX_ADDR + 0x4) {
        /* 16550 reg1：DLAB=1 时 DLM，否则 IER */
        ret = (u->lcr & 0x80) ? u->dlm : u->ier;
    } else if (addr == UART_TX_ADDR + 0x8) {
        /* 16550 IIR，按优先级报告：RDA(0x04) > THRE(0x02)，无则 0xC1。
           不报 THRE 的话 8250 驱动开 ETBEI 后等不到续发中断，
           TX 停摆（表现为输出要按键才一截一截出来） */
        if (rx_has_data(u) && (u->ier & 1))
            ret = 0x04;
        else if (u->ier & 2)
            ret = 0x02;
        else
            ret = 0xC1;
    } else if (addr == UART_TX_ADDR + 0xC) {
        /* 16550 LCR */
        ret = u->lcr;
    } else if (addr == UART_SR_ADDR) {
        ret  = rx_has_data(u) ? 1 : 0;  /* bit0 = rx_valid */
        ret |= rx_is_full(u)  ? 2 : 0;  /* bit1 = rx_full  */
        ret |= 0x60;  /* bit5=THR空, bit6=TEMT (NS16550 兼容) */
    } else if (addr == UART_IER_ADDR) {
        /* 0x8218 = 16550 reg6 MSR：DSR|CTS 常有效、无 delta */
        ret = 0x30;
    } else if (addr == UART_RX_CNT_ADDR) {
        ret = (u->rx_head - u->rx_tail) % RX_BUF_SIZE;
    }

    (void)size;
    return ret;
}

/* ---------- write: 写 UART 寄存器 ---------- */
static void uart_write(Peripheral *p, uint32_t addr, uint32_t value,
                       int size, struct CPU *cpu) {
    UART_Priv *u = (UART_Priv *)p->priv;

    if (addr == UART_TX_ADDR) {
        if (u->lcr & 0x80)
            u->dll = value & 0xFF;        /* DLAB=1: 写 DLL 波特率锁存 */
        else {
            putchar(value & 0xFF);        /* DLAB=0: 发送 */
            fflush(stdout);
        }
    } else if (addr == UART_TX_ADDR + 0x4) {
        if (u->lcr & 0x80)
            u->dlm = value & 0xFF;        /* DLAB=1: 写 DLM */
        else {
            /* DLAB=0: 16550 IER，bit0=ERBFI(RX)，bit1=ETBEI(THRE) */
            u->ier = value & 0x3;
            uart_update_irq(p, cpu);
        }
    } else if (addr == UART_TX_ADDR + 0xC) {
        u->lcr = value & 0xFF;            /* 16550 LCR */
    } else if (addr == UART_IER_ADDR) {
        /* 0x8218 自定义 IER 地址（RTOS 固件用），只控 RX 位 */
        u->ier = (u->ier & ~1u) | (value & 1);
        uart_update_irq(p, cpu);   /* 重新评估中断状态 */
    }

    (void)size;
}

/* ---------- tick: UART 不需要 per-cycle 操作 ---------- */
static void uart_tick(Peripheral *p, struct CPU *cpu) {
    (void)p;
    (void)cpu;
}

/* ---------- 公共 API ---------- */

Peripheral *uart_create(void) {
    Peripheral *p = calloc(1, sizeof(Peripheral));
    UART_Priv  *u = calloc(1, sizeof(UART_Priv));

    u->ier = 0;  /* 中断默认关闭，CPU 通过写 IER 寄存器使能 */

    p->name  = "uart";
    p->match = uart_match;
    p->read  = uart_read;
    p->write = uart_write;
    p->tick  = uart_tick;
    p->priv  = u;

    return p;
}

void uart_inject_char(Peripheral *p, uint8_t ch, struct CPU *cpu) {
    UART_Priv *u = (UART_Priv *)p->priv;
    if (!rx_is_full(u)) {
        u->rx_buf[u->rx_head] = ch;
        u->rx_head = (u->rx_head + 1) % RX_BUF_SIZE;
        uart_update_irq(p, cpu);
    }
    /* 缓冲区满时丢弃，避免阻塞宿主机 */
}
