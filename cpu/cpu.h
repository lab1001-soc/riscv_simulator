/*
 * cpu.h - RISC-V CPU 核心结构体定义
 * 支持 RV32IMACF 完整指令集
 */

#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include "config.h"
#include "mem.h"
#include "peripheral.h"
#include "mmu.h"

#define NUM_REGS 32
#define NUM_FREGS 32
#define MAX_PERIPHERALS 8

/* 浮点数联合体 */
typedef union {
    float f;
    uint32_t u;
} Float32;

/* 指令类型定义 */
typedef struct {
    uint32_t opcode:7, rd:5, funct3:3, rs1:5, rs2:5, funct7:7;
} RType;

typedef struct {
    uint32_t opcode:7, rd:5, funct3:3, rs1:5;
    int32_t imm;  /* 立即数需要符号扩展，不能用12位位域 */
} IType;

typedef struct {
    uint32_t opcode:7, imm_low:5, funct3:3, rs1:5, rs2:5, imm_high:7;
} SType;

typedef struct {
    uint32_t opcode:7, rd:5, rm:3, rs1:5, rs2:5, fmt:2, rs3:5;
} R4Type;  /* F 扩展融合乘加指令 */

/* 压缩指令 quadrant 定义 */
#define C_QUADRANT_0  0x00  /* C.ADDI4SPN, C.LW, C.SW */
#define C_QUADRANT_1  0x01  /* C.ADDI, C.JAL, C.LI, C.J, C.BEQZ */
#define C_QUADRANT_2  0x02  /* C.SLLI, C.LWSP, C.JR, C.MV, C.ADD */
#define C_QUADRANT_3  0x03  /* 32 位标准指令 */

/* CPU 状态结构体 - 包含所有寄存器和状态 */
typedef struct CPU {
    union {
        uint32_t regs[NUM_REGS];  /* 数组访问：cpu->regs[idx] */
        struct {
            uint32_t zero;        /* x0 - 硬连线为 0 */
            uint32_t ra;          /* x1 - 返回地址 */
            uint32_t sp;          /* x2 - 栈指针 */
            uint32_t gp;          /* x3 - 全局指针 */
            uint32_t tp;          /* x4 - 线程指针 */
            uint32_t t0, t1, t2;  /* x5-x7 - 临时寄存器 */
            uint32_t s0, s1;      /* x8-x9 - 保存寄存器 */
            uint32_t a0, a1, a2, a3, a4, a5, a6, a7;  /* x10-x17 - 参数寄存器 */
            uint32_t s2, s3, s4, s5, s6, s7, s8, s9, s10, s11;  /* x18-x27 */
            uint32_t t3, t4, t5, t6;  /* x28-x31 - 临时寄存器 */
        };  /* 别名访问：cpu->sp, cpu->ra 等 */
    };  /* 匿名 union，成员可直接访问 */
    union {
        /* 浮点寄存器 f0-f31 */
        Float32 fregs[NUM_FREGS];
        struct {
            Float32 ft0, ft1, ft2, ft3, ft4, ft5, ft6, ft7;
            Float32 fs0, fs1;
            Float32 fa0, fa1, fa2, fa3, fa4, fa5, fa6, fa7;
            Float32 fs2, fs3, fs4, fs5, fs6, fs7, fs8, fs9, fs10, fs11;
            Float32 ft8, ft9, ft10, ft11, ft12, ft13, ft14, ft15;
        };
    };
    /* 程序计数器 */
    uint32_t pc;

    /* 指令内存 (IMEM) */
    uint32_t imem[IMEM_SIZE / 4];

    /* 数据内存 (DMEM) */
    uint32_t dmem[DMEM_SIZE / 4];

    /* Linux 内存 (LMEM) — 堆分配, 大容量 */
    uint32_t *lmem;
    uint32_t lmem_size;   /* 实际分配的字节数 */

    uint32_t breakpoint[4];
    uint32_t brkctrl[4];

    /* 浮点控制状态寄存器 */
    uint32_t fcsr;

    /* 机器模式 CSR */
    uint32_t mstatus;    /* 0x300 - 机器状态寄存器 */
    uint32_t mtvec;      /* 0x305 - 机器陷阱向量基址 */
    uint32_t mscratch;   /* 0x340 - 机器暂存寄存器 */
    uint32_t mepc;       /* 0x341 - 机器异常PC */
    uint32_t mcause;     /* 0x342 - 机器陷阱原因 */
    uint32_t mtval;      /* 0x343 - 机器陷阱值 */
    uint32_t mie;        /* 0x304 - 机器中断使能 */
    uint32_t mip;        /* 0x344 - 机器中断挂起 */

    /* 原子操作保留地址 */
    int has_reservation;
    uint32_t reservation_addr;

    /* ===== 特权级 ===== */
    uint32_t privilege;     /* 当前特权级: 0=U, 1=S, 3=M */

    /* ===== 异常/中断委托 ===== */
    uint32_t medeleg;       /* 0x302 - 异常委托 */
    uint32_t mideleg;       /* 0x303 - 中断委托 */

    /* ===== S-mode CSR ===== */
    uint32_t stvec;         /* 0x105 - S-mode 陷阱向量 */
    uint32_t sepc;          /* 0x141 - S-mode 异常 PC */
    uint32_t scause;        /* 0x142 - S-mode 陷阱原因 */
    uint32_t stval;         /* 0x143 - S-mode 陷阱值 */
    uint32_t sscratch;      /* 0x140 - S-mode 暂存寄存器 */
    uint32_t sie;           /* 0x104 - S-mode 中断使能 */
    uint32_t sip;           /* 0x144 - S-mode 中断挂起 */
    uint32_t satp;          /* 0x180 - 地址翻译 */

    /* MMU TLB (三级: ITLB + DTLB + STLB) */
    TLB_Entry itlb[ITLB_SIZE];
    TLB_Entry dtlb[DTLB_SIZE];
    TLB_Entry stlb[STLB_SIZE];

    /* MMU 流水线延迟：csrw satp 后前几条指令不经过新页表 */
    int mmu_skip;

    /* 运行状态 */
    int halted;
    int step;
    int monitor_mode;    /* 0=UART, 1=Monitor (键盘输入目标) */
    int quit;            /* 退出标志 */
    int dbg_cmd_mode;    /* 1=扩展命令输入模式 */
    char dbg_buf[64];    /* 命令缓冲区 */
    int dbg_pos;         /* 缓冲区当前位置 */
    uint32_t trace_buf[32];
    uint32_t trace_idx;
    uint64_t cycles;

    /* 外设列表 */
    Peripheral *peripherals[MAX_PERIPHERALS];
    int num_peripherals;
} CPU;

/* 指令操作码定义 */
#define OP_LOAD     0x03
#define OP_STORE    0x23
#define OP_OP_IMM   0x13
#define OP_OP       0x33
#define OP_LUI      0x37
#define OP_AUIPC    0x17
#define OP_BRANCH   0x63
#define OP_JALR     0x67
#define OP_JAL      0x6F
#define OP_FENCE    0x0F
#define OP_SYSTEM   0x73

/* M 扩展 funct3 */
#define F3_MUL    0x0
#define F3_MULH   0x1
#define F3_MULHSU 0x2
#define F3_MULHU  0x3
#define F3_DIV    0x4
#define F3_DIVU   0x5
#define F3_REM    0x6
#define F3_REMU   0x7

/* A 扩展 funct3 */
#define F3_AMO    0x02

/* CSR 地址定义 */
#define CSR_FFLAGS   0x001  /* 浮点异常标志 */
#define CSR_FRM      0x002  /* 浮点舍入模式 */
#define CSR_FCSR     0x003  /* 浮点控制状态寄存器 (FCSR = FRM<<5 | FFLAGS) */

/* S-mode CSR */
#define CSR_SSTATUS  0x100  /* S-mode 状态寄存器 (mstatus 的影子) */
#define CSR_SIE      0x104  /* S-mode 中断使能 */
#define CSR_STVEC    0x105  /* S-mode 陷阱向量 */
#define CSR_SSCRATCH 0x140  /* S-mode 暂存寄存器 */
#define CSR_SEPC     0x141  /* S-mode 异常 PC */
#define CSR_SCAUSE   0x142  /* S-mode 陷阱原因 */
#define CSR_STVAL    0x143  /* S-mode 陷阱值 */
#define CSR_SIP      0x144  /* S-mode 中断挂起 */
#define CSR_SATP     0x180  /* S-mode 地址翻译 */

/* M-mode CSR */
#define CSR_MSTATUS  0x300  /* 机器状态寄存器 */
#define CSR_MEDELEG  0x302  /* 异常委托 */
#define CSR_MIDELEG  0x303  /* 中断委托 */
#define CSR_MIE      0x304  /* 机器中断使能 */
#define CSR_MTVEC    0x305  /* 机器陷阱向量基址 */
#define CSR_MSCRATCH 0x340  /* 机器暂存寄存器 */
#define CSR_MEPC     0x341  /* 机器异常PC */
#define CSR_MCAUSE   0x342  /* 机器陷阱原因 */
#define CSR_MTVAL    0x343  /* 机器陷阱值 */
#define CSR_MIP      0x344  /* 机器中断挂起 */

/* Machine Information Registers */
#define CSR_MISA      0x301  /* ISA 与扩展 */
#define CSR_MVENDORID 0xF11  /* 厂商 ID */
#define CSR_MARCHID   0xF12  /* 架构 ID */
#define CSR_MIMPID    0xF13  /* 实现 ID */
#define CSR_MHARTID   0xF14  /* 硬件线程 ID */

/* Timer CSRs (只读) */
#define CSR_TIME      0xC01  /* mtime 低 32 位 */
#define CSR_TIMEH     0xC81  /* mtime 高 32 位 */

/* mstatus 位定义 */
#define MSTATUS_SIE   (1u << 1)     /* S-mode 全局中断使能 */
#define MSTATUS_MIE   (1u << 3)     /* M-mode 全局中断使能 */
#define MSTATUS_SPIE  (1u << 5)     /* S-mode 中断前的 SIE */
#define MSTATUS_MPIE  (1u << 7)     /* M-mode 中断前的 MIE */
#define MSTATUS_SPP   (1u << 8)     /* S-mode 进入陷阱前的特权级: 0=U, 1=S */
#define MSTATUS_MPP   (3u << 11)    /* M-mode 进入陷阱前的特权级 */
#define MSTATUS_FS    (3u << 13)    /* 浮点状态 */
#define MSTATUS_XS    (3u << 15)    /* 扩展状态 */
#define MSTATUS_MPRV  (1u << 17)    /* 内存权限修改 */
#define MSTATUS_SUM   (1u << 18)    /* S-mode 可访问 U-mode 内存 */
#define MSTATUS_MXR   (1u << 19)    /* 可执行页可读 */
#define MSTATUS_TVM   (1u << 20)    /* 陷入 SFENCE.VMA/SATP */
#define MSTATUS_TSR   (1u << 22)    /* 陷入 SRET */
#define MSTATUS_SD    (1u << 31)    /* 状态脏 (FS 或 XS 被修改) */

/* mip / mie 中断位定义 */
#define MIP_MSIP      (1u << 3)    /* 机器软件中断 */
#define MIP_MTIP      (1u << 7)    /* 机器定时器中断 */
#define MIP_MEIP      (1u << 11)   /* 机器外部中断 */
#define MIP_SEIP      (1u << 9)    /* S-mode 外部中断（hart S-ext 引脚，S 态经 sip 视图读） */

/* sip / sie 中断位定义 */
#define SIP_SSIP      (1u << 1)    /* S-mode 软件中断 */
#define SIP_STIP      (1u << 5)    /* S-mode 定时器中断 */
#define SIP_SEIP      (1u << 9)    /* S-mode 外部中断 */

/* 特权级定义 */
#define PRIV_U  0
#define PRIV_S  1
#define PRIV_M  3

/* 异常原因码 */
#define CAUSE_MISALIGNED_FETCH    0x0
#define CAUSE_FETCH_ACCESS        0x1
#define CAUSE_ILLEGAL_INSTRUCTION 0x2
#define CAUSE_BREAKPOINT          0x3
#define CAUSE_MISALIGNED_LOAD     0x4
#define CAUSE_LOAD_ACCESS         0x5
#define CAUSE_MISALIGNED_STORE    0x6
#define CAUSE_STORE_ACCESS        0x7
#define CAUSE_USER_ECALL          0x8
#define CAUSE_SUPERVISOR_ECALL    0x9
#define CAUSE_HYPERVISOR_ECALL    0xa
#define CAUSE_MACHINE_ECALL       0xb
#define CAUSE_INSTRUCTION_PAGE_FAULT 0xc
#define CAUSE_LOAD_PAGE_FAULT     0xd
#define CAUSE_STORE_PAGE_FAULT    0xf

/* MRET 指令编码 */
#define MRET_INSTR    0x30200073

/* SRET 指令编码 */
#define SRET_INSTR    0x10200073

/* WFI 指令编码 */
#define WFI_INSTR     0x10500073

/* CSR 指令 funct3 */
#define F3_CSRRW   0x1
#define F3_CSRRS   0x2
#define F3_CSRRC   0x3
#define F3_CSRRWI  0x5
#define F3_CSRRSI  0x6
#define F3_CSRRCI  0x7

/* F 扩展 opcode */
#define OP_LOAD_FP   0x07
#define OP_STORE_FP  0x27
#define OP_FMADD     0x43
#define OP_FMSUB     0x47
#define OP_FNMSUB    0x4B
#define OP_FNMADD    0x4F
#define OP_OP_FP     0x53

/* 函数声明 */
void cpu_init(CPU *cpu);
int  cpu_load_binary_at(CPU *cpu, const char *filename, uint32_t base_addr);
int is_compressed(uint16_t instr_low);
uint32_t cpu_fetch(CPU *cpu, int *instr_len);
uint32_t expand_compressed(uint16_t instr);
int cpu_decode(CPU *cpu, uint32_t instr, RType *r, IType *i, SType *s, R4Type *r4);
int cpu_execute(CPU *cpu, uint32_t instr, int instr_len);
void cpu_writeback(CPU *cpu, uint32_t rd, uint32_t value);
void cpu_run(CPU *cpu);
void cpu_dump(CPU *cpu, int dump_type);
void cpu_trap(CPU *cpu, uint32_t cause, uint32_t tval);
int8_t cpu_halt_det(CPU *cpu,uint32_t rw,uint32_t addr);
int  cpu_check_interrupts(CPU *cpu);
void handle_host_input(CPU *cpu);
/* 符号扩展辅助函数 */
static inline int32_t sign_extend(uint32_t val, int bits) {
    if (val & (1u << (bits - 1)))
        return val | (~0u << bits);
    return val;
}

#endif /* CPU_H */
