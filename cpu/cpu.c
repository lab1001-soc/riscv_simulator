/*
 * cpu.c - CPU 核心初始化和运行控制
 */

#include "cpu.h"
#include "uart.h"
#include "timer.h"
#include "software.h"
#include "plic.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef _WIN32
#include <conio.h>    /* _kbhit, _getch */
#include <windows.h>  /* Sleep */
#else
#include <unistd.h>   /* read, usleep */
#include <termios.h>  /* tcgetattr/tcsetattr */
#include <fcntl.h>    /* fcntl O_NONBLOCK */
#endif

/* 外部函数声明 */
extern int is_compressed(uint16_t instr_low);
extern uint32_t cpu_fetch(CPU *cpu, int *instr_len);
extern int cpu_execute(CPU *cpu, uint32_t instr, int instr_len);
extern void cpu_dump(CPU *cpu, int dump_type);

void printv1(uint8_t *buf, uint32_t len, char *s)
{
    uint32_t i = 0;
    printf("%s:", s);
    for (i = 0; i < len; i++)
    {
        printf("%02x ", buf[i]);
    }
    printf("\r\n");
}

void printv(uint8_t *buf, uint32_t len, char *s)
{
    uint32_t i = 0;
    uint32_t n = 0;
    printf("%s:", s);
    for (i = 0; i < len; i++)
    {
        if (i % 16 == 0)
        {
            printf("\n%08x:", n);
            n += 16;
        }
        printf("%02x ", buf[i]);
    }
    printf("\r\n");
}

/**
 * cpu_init - 初始化 CPU 状态
 * @cpu: CPU 状态结构体指针
 *
 * 将所有寄存器、内存和状态清零
 */
void cpu_init(CPU *cpu) {
    /* 清零通用寄存器 */
    memset(cpu->regs, 0, sizeof(cpu->regs));

    /* 清零浮点寄存器 */
    memset(cpu->fregs, 0, sizeof(cpu->fregs));

    /* 清零指令内存 (IMEM) */
    memset(cpu->imem, 0, sizeof(cpu->imem));

    /* 清零数据内存 (DMEM) */
    memset(cpu->dmem, 0, sizeof(cpu->dmem));

    /* 分配 Linux 内存 (LMEM) — 堆上分配, 支持大容量 */
    cpu->lmem_size = LMEM_SIZE;
    cpu->lmem = calloc(1, cpu->lmem_size);
    if (!cpu->lmem) {
        fprintf(stderr, "[CPU] LMEM 分配失败: %d MB\n", LMEM_SIZE / (1024*1024));
        exit(1);
    }

    /* 初始化 PC */
    cpu->pc = PC_RESET_ADDR;

    /* 清零 FCSR */
    cpu->fcsr = 0;

    /* 清除原子操作保留状态 */
    cpu->has_reservation = 0;
    cpu->reservation_addr = 0;

    /* 运行状态 */
    cpu->halted = 0;
    cpu->step = 0;
    cpu->cycles = 0;

    /* 初始化机器模式 CSR */
    cpu->mstatus = MSTATUS_MPP;   /* MPP = M-mode (3)，MIE = 0 */
    cpu->mtvec = 0;
    cpu->mepc = 0;
    cpu->mcause = 0;
    cpu->mscratch = 0;
    cpu->mtval = 0;
    cpu->mie = 0;
    cpu->mip = 0;

    /* 特权级：初始为 M-mode */
    cpu->privilege = PRIV_M;

    /* 异常/中断委托：默认不委托（全部走 M-mode） */
    cpu->medeleg = 0;
    cpu->mideleg = 0;

    /* S-mode CSR 初始化为 0 */
    cpu->stvec    = 0;
    cpu->sepc     = 0;
    cpu->scause   = 0;
    cpu->stval    = 0;
    cpu->sscratch = 0;
    cpu->sie      = 0;
    cpu->sip      = 0;
    cpu->satp     = 0;
    cpu->mmu_skip = 0;

    /* 初始化运行控制 */
    cpu->monitor_mode = 0;  /* 默认 UART 模式 */
    cpu->quit = 0;

    /* 注册外设 */
    cpu->num_peripherals = 0;
    cpu->peripherals[cpu->num_peripherals++] = uart_create();
    cpu->peripherals[cpu->num_peripherals++] = timer_create();
    cpu->peripherals[cpu->num_peripherals++] = software_create();
    cpu->peripherals[cpu->num_peripherals++] = plic_create();   /* idx 3 */

    /* x0 硬连线为 0 */
    cpu->regs[0] = 0;

    cpu->breakpoint[0] = 0x10009e4;
    cpu->brkctrl[0] = 0x0; //1: exe 2: r/w 3: write

    cpu->breakpoint[1] = 0x100152a;
    cpu->brkctrl[1] = 0x00;

    cpu->breakpoint[2] = 0x1feb8;
    cpu->brkctrl[2] = 0x00;

    cpu->breakpoint[3] = 0x1feb8;
    cpu->brkctrl[3] = 0x00;

    cpu->trace_idx = 0;

    cpu->dbg_cmd_mode = 0;
}

/**
 * cpu_trap - 触发陷阱（支持委托到 S-mode）
 * @cpu: CPU 状态结构体指针
 * @cause: 异常/中断原因码 (bit31=1 表示中断)
 * @tval: 陷阱值 (出错的地址或指令编码)
 *
 * 委托逻辑：
 * - M-mode: 总是由 M-mode 处理
 * - S/U-mode: 根据 medeleg/mideleg 决定是否委托到 S-mode
 */
void cpu_trap(CPU *cpu, uint32_t cause, uint32_t tval) {
    uint32_t is_irq = cause >> 31;    /* bit31=1 表示中断 */
    uint32_t cause_code = cause & 0x7FFFFFFF;

    /* M-mode: 总是由 M-mode 处理 */
    if (cpu->privilege == PRIV_M) {
        goto m_trap;
    }

    /* S/U-mode: 检查是否委托到 S-mode */
    if (is_irq) {
        /* 中断：检查 mideleg */
        if (cpu->mideleg & (1u << cause_code)) {
            goto s_trap;
        }
    } else {
        /* 异常：检查 medeleg */
        if (cause_code < 16 && (cpu->medeleg & (1u << cause_code))) {
            goto s_trap;
        }
    }

    /* 不委托 → M-mode trap */
    goto m_trap;

m_trap:
    cpu->mepc    = cpu->pc;
    cpu->mcause  = cause;
    cpu->mtval   = tval;

    /* 保存 MIE 到 MPIE，然后关中断 */
    if (cpu->mstatus & MSTATUS_MIE)
        cpu->mstatus |= MSTATUS_MPIE;
    else
        cpu->mstatus &= ~MSTATUS_MPIE;
    cpu->mstatus &= ~MSTATUS_MIE;

    /* MPP = 当前特权级 */
    cpu->mstatus = (cpu->mstatus & ~MSTATUS_MPP)
                 | (cpu->privilege << 11);
    cpu->privilege = PRIV_M;

    /* 跳转到陷阱入口 */
    {
        uint32_t base = cpu->mtvec & ~0x3u;
        uint32_t mode = cpu->mtvec & 0x3u;
        if (mode == 0)
            cpu->pc = base;
        else
            cpu->pc = base + (cause_code << 2);
    }

#ifdef DEBUG_TRAP
    printf("[M-TRAP] cause=0x%08x, tval=0x%08x, mepc=0x%08x → pc=0x%08x\n",
           cause, tval, cpu->mepc, cpu->pc);
#endif
    return;

s_trap:
    cpu->sepc    = cpu->pc;
    cpu->scause  = cause;
    cpu->stval   = tval;

    /* 保存 SIE 到 SPIE，然后关 SIE */
    if (cpu->mstatus & MSTATUS_SIE)
        cpu->mstatus |= MSTATUS_SPIE;
    else
        cpu->mstatus &= ~MSTATUS_SPIE;
    cpu->mstatus &= ~MSTATUS_SIE;

    /* SPP = 当前特权级 (0=U, 1=S) */
    if (cpu->privilege == PRIV_S)
        cpu->mstatus |= MSTATUS_SPP;
    else
        cpu->mstatus &= ~MSTATUS_SPP;

    cpu->privilege = PRIV_S;

    /* 跳转到 stvec */
    {
        uint32_t base = cpu->stvec & ~0x3u;
        uint32_t mode = cpu->stvec & 0x3u;
        if (mode == 0)
            cpu->pc = base;
        else
            cpu->pc = base + (cause_code << 2);
    }

#ifdef DEBUG_TRAP
    printf("[S-TRAP] cause=0x%08x, tval=0x%08x, sepc=0x%08x → pc=0x%08x\n",
           cause, tval, cpu->sepc, cpu->pc);
#endif
}

/**
 * cpu_load_program - 加载程序到指令内存
 * @cpu: CPU 状态结构体指针
 * @program: 程序数据指针
 * @size: 程序大小 (字节)
 *
 * 返回：0 表示成功，-1 表示失败
 */
int cpu_load_program(CPU *cpu, const uint8_t *program, size_t size) {
    /* 使用统一内存模块加载到 IMEM 基地址 */
    return mem_load_program(cpu, program, size, IMEM_BASE_ADDR);
}

/**
 * cpu_load_binary - 从文件加载二进制程序
 * @cpu: CPU 状态结构体指针
 * @filename: 文件名
 *
 * 返回：0 表示成功，-1 表示失败
 */
int cpu_load_binary(CPU *cpu, const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "[CPU] 无法打开文件：%s\n", filename);
        return -1;
    }

    /* 获取文件大小 */
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (size > IMEM_SIZE) {
        fprintf(stderr, "[CPU] 文件太大：%ld 字节 (最大 %d)\n", size, IMEM_SIZE);
        fclose(f);
        return -1;
    }

    /* 读取文件 */
    uint8_t *prog = malloc(size);
    if (!prog) {
        fprintf(stderr, "[CPU] 内存分配失败\n");
        fclose(f);
        return -1;
    }

    size_t read = fread(prog, 1, size, f);
    fclose(f);

    if (read != (size_t)size) {
        fprintf(stderr, "[CPU] 读取文件失败\n");
        free(prog);
        return -1;
    }

    /* 加载到内存 (使用统一内存模块) */
    int ret = mem_load_program(cpu, prog, size, IMEM_BASE_ADDR);
    free(prog);

    printf("load binary 0x%1lx bytes\n", size);

    return ret;
}

/**
 * cpu_load_binary_at - 从文件加载二进制程序到指定地址
 * @cpu: CPU 状态
 * @filename: 文件名
 * @base_addr: 加载目标物理地址 (支持 IMEM/DMEM/LMEM)
 *
 * 返回：0 成功, -1 失败
 */
int cpu_load_binary_at(CPU *cpu, const char *filename, uint32_t base_addr) {
    FILE *f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "[CPU] 无法打开文件：%s\n", filename);
        return -1;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    uint8_t *prog = malloc(size);
    if (!prog) {
        fprintf(stderr, "[CPU] 内存分配失败\n");
        fclose(f);
        return -1;
    }

    size_t read = fread(prog, 1, size, f);
    fclose(f);

    if (read != (size_t)size) {
        fprintf(stderr, "[CPU] 读取文件失败\n");
        free(prog);
        return -1;
    }

    int ret = mem_load_program(cpu, prog, size, base_addr);
    free(prog);

    printf("  loaded %ld bytes → 0x%08x\n", size, base_addr);
    return ret;
}

/*
rw: 0x00 exe
    0x01 读取
    0x02 写入
addr: rw addr
*/
int8_t cpu_halt_det(CPU *cpu,uint32_t rw,uint32_t addr)
{
    /*
     brkctrl 0x01 exe
     brkctrl 0x02 r/w
     brkctrl 0x03 write
    */
    int8_t halt = 0;
    for(int i = 0; i < 4 && !halt; i++)
    {
        switch(rw)
        {
            case 0x00:
                if(cpu->pc == cpu->breakpoint[i] && (cpu->brkctrl[i] & 0x03) == 0x01 ) //exe
                {
                    printf("\r\n[0x%08x] exe halt!! ", cpu->pc);
                    halt = 1;
                }
                break;
            case 0x01:
                if(addr == cpu->breakpoint[i] && (cpu->brkctrl[i] & 0x03) == 0x02 ) //读取
                {
                    printf("\r\n[0x%08x] read:%08x halt!! ", cpu->pc,addr);
                    halt = 1;
                }
                break;
            case 0x02:
                if(addr == cpu->breakpoint[i] && ((cpu->brkctrl[i] & 0x03) == 0x03 ||
                                                  (cpu->brkctrl[i] & 0x03) == 0x02)) //写入
                {
                    printf("\r\n[0x%08x] write:%08x halt!! ", cpu->pc,addr);
                    halt = 1;
                }
                break;
            default:
                break;
        }
    }
    if(halt)
        cpu->halted = 1;

    return halt;
}


int8_t cpu_cmd_line(CPU *cpu)
{
    char cmd;
    if(cpu->halted || cpu->step)
    {
        cmd = _getch();
        switch(cmd)
        {
            case 'q':
                printf("退出\n");
                return -1;
            case 'c':
                cpu->step = 0;
                break;
            case 's':
                cpu->step = 1;
                printf("单步执行: 0x%08x\n", cpu->pc);

                break;
            default:
                break;
        }

        cpu->halted = 0;
    }
    return 0;
}

void cpu_trace(CPU *cpu)
{
    static uint32_t last_pc = 0;
    int32_t distance = cpu->pc - last_pc;
    if(cpu->pc != last_pc && (distance > 4 || distance < 0))
    {
        cpu->trace_buf[cpu->trace_idx] = cpu->pc;
        cpu->trace_idx++;
        if(cpu->trace_idx >= sizeof(cpu->trace_buf)/sizeof(cpu->trace_buf[0])-1)
            cpu->trace_idx = 0;
    }
    last_pc = cpu->pc;
}

/* ==================== 中断与键盘处理 ==================== */

/**
 * cpu_check_interrupts - 检查挂起的中断
 * 返回：1 表示中断已触发，0 表示无中断
 *
 * 按当前特权级检查：
 * - M-mode: 检查 mip & mie，需要 MSTATUS_MIE
 * - S/U-mode: 优先检查 sip & sie，需要 MSTATUS_SIE（S-mode 时）
 */
int cpu_check_interrupts(CPU *cpu) {
    if (cpu->privilege == PRIV_M) {
        /* M-mode: 只处理未委托的中断 (mideleg=1 的中断由 S-mode 处理) */
        if (!(cpu->mstatus & MSTATUS_MIE))
            return 0;
        uint32_t pending = cpu->mip & cpu->mie & ~cpu->mideleg;
        if (!pending)
            return 0;

        /* 优先级: MEI(11) > MSI(3) > MTI(7) */
        uint32_t irq;
        if (pending & MIP_MEIP)
            irq = 11;
        else if (pending & MIP_MSIP)
            irq = 3;
        else if (pending & MIP_MTIP)
            irq = 7;
        else
            return 0;

        cpu_trap(cpu, 0x80000000 | irq, 0);
        return 1;
    } else {
        /* S/U-mode: 检查 S-mode 中断 */
        if (!(cpu->mstatus & MSTATUS_SIE) && cpu->privilege == PRIV_S)
            return 0;  /* S-mode 下 SIE=0 时屏蔽 */
        uint32_t pending = cpu->sip & cpu->sie;
        if (!pending)
            return 0;

        /* 优先级: SEI(9) > SSI(1) > STI(5) */
        uint32_t irq;
        if (pending & SIP_SEIP)
            irq = 9;
        else if (pending & SIP_SSIP)
            irq = 1;
        else if (pending & SIP_STIP)
            irq = 5;
        else
            return 0;

        cpu_trap(cpu, 0x80000000 | irq, 0);
        return 1;
    }
}

/**
 * cpu_dbg_exec - 解析并执行扩展命令 (e 命令，argc/argv 解析)
 */
static void cpu_dbg_exec(CPU *cpu) {
    char *buf = cpu->dbg_buf;
    printf("\n");

    /* 按空格分割为 argc/argv */
    char *argv[8];
    int argc = 0;
    char *p = buf;
    while (*p) {
        while (*p == ' ') p++;
        if (*p == '\0') break;
        argv[argc++] = p;
        if (argc >= 8) break;
        while (*p && *p != ' ') p++;
        if (*p) *p++ = '\0';
    }
    if (argc < 2) goto usage;

    /* e b <addr> [ctrl] — 断点设置 */
    if (strcmp(argv[1], "b") == 0) {
        if (argc < 3) goto usage;
        cpu->breakpoint[0] = strtoul(argv[2], NULL, 16);
        if (argc >= 4)
            cpu->brkctrl[0] = strtoul(argv[3], NULL, 16);
        else
            cpu->brkctrl[0] = 1;  /* 默认 exe 断点 */
        printf("  break0: addr=0x%08x ctrl=%d\n",
               cpu->breakpoint[0], cpu->brkctrl[0]);
        return;
    }

    /* e <addr>l<len> — 内存读取，在 argv[1] 中找 'l' 分隔 */
    {
        char *lpos = strchr(argv[1], 'l');
        if (lpos) {
            *lpos = '\0';
            uint32_t addr = strtoul(argv[1], NULL, 16);
            int len = (int)strtoul(lpos + 1, NULL, 16);
            if (len > 1024) len = 1024;
            printf("  mem @0x%08x (%d bytes):\n", addr, len);
            for (int i = 0; i < len; i += 16) {
                printf("  %08x:", addr + i);
                for (int j = 0; j < 16 && (i + j) < len; j++)
                    printf(" %02x", mem_read(cpu, addr + i + j, 1));
                printf("\n");
            }
            return;
        }
    }

usage:
    printf("  e <addr>l<len>      - memory dump\n");
    printf("  e b <addr> [ctrl]   - set break0\n");
}

#ifndef _WIN32
static int host_tty_init = 0;
static struct termios host_tty_orig;

static void host_tty_restore(void) {
    tcsetattr(0, TCSANOW, &host_tty_orig);
}
#endif

/**
 * handle_host_input - 统一键盘输入处理（非阻塞）
 * Ctrl+]: 切换 Monitor / UART 模式
 */
void handle_host_input(CPU *cpu) {
    /* 每 INPUT_CHECK_INTERVAL 条指令检查一次键盘，_kbhit() 是系统调用 */
    static uint32_t input_timer = 0;
    if (++input_timer < INPUT_CHECK_INTERVAL) return;
    input_timer = 0;

#ifdef _WIN32
    char c;
    if (_kbhit()) {
        c = (char)_getch();
    } else {
        /* 管道 stdin（自动化测试）：_kbhit 只管控制台，管道用 PeekNamedPipe 探测 */
        DWORD avail = 0;
        HANDLE hstdin = GetStdHandle(STD_INPUT_HANDLE);
        if (!PeekNamedPipe(hstdin, NULL, 0, NULL, &avail, NULL) || avail == 0)
            return;
        DWORD rd = 0;
        if (!ReadFile(hstdin, &c, 1, &rd, NULL) || rd != 1)
            return;
    }
#else
    /* POSIX (Linux/macOS)：终端设 cbreak + 非阻塞 read（对 tty 和管道通用）*/
    if (!host_tty_init) {
        host_tty_init = 1;
        if (tcgetattr(0, &host_tty_orig) == 0) {
            struct termios t = host_tty_orig;
            /* 原始键码交付（对齐 Windows _getch）：关行缓冲/回显/信号 */
            t.c_lflag &= ~(tcflag_t)(ICANON | ECHO | ISIG);
            t.c_cc[VMIN]  = 0;
            t.c_cc[VTIME] = 0;
            tcsetattr(0, TCSANOW, &t);
            atexit(host_tty_restore);
        }
        fcntl(0, F_SETFL, fcntl(0, F_GETFL, 0) | O_NONBLOCK);
    }
    char c;
    if (read(0, &c, 1) != 1)
        return;
#endif

    /* Ctrl+] → 切换 Monitor / UART 模式 (0x1D，终端不拦截) */
    if (c == 0x1D) {
        cpu->monitor_mode = !cpu->monitor_mode;
        if (cpu->monitor_mode) {
            printf("\n[MONITOR] PC=0x%08x | c=继续 s=单步 h=暂停 r=regs d=regs+CSR p=PC q=退出\n",
                   cpu->pc);
        } else {
            printf("\n[UART]\n");
        }
        return;
    }

    /* Monitor 模式：调试命令 */
    if (cpu->monitor_mode) {
        /* 扩展命令输入模式：逐字符收集，回车执行 */
        if (cpu->dbg_cmd_mode) {
            if (c == '\r' || c == '\n') {
                cpu_dbg_exec(cpu);
                cpu->dbg_cmd_mode = 0;
            } else if (c == '\b' || c == 127) {
                if (cpu->dbg_pos > 0) {
                    cpu->dbg_pos--;
                    printf("\b \b");
                    fflush(stdout);
                }
            } else if (cpu->dbg_pos < 63) {
                cpu->dbg_buf[cpu->dbg_pos++] = c;
                putchar(c);
                fflush(stdout);
            }
            return;
        }

        switch (c) {
            case 'c':
                cpu->halted = 0;
                cpu->step = 0;
                printf("[继续] PC=0x%08x\n", cpu->pc);
                break;
            case 's':
                cpu->halted = 0;
                cpu->step = 1;
                printf("[单步]\n");
                break;
            case 'h':
                cpu->halted = 1;
                printf("[暂停] PC=0x%08x\n", cpu->pc);
                break;
            case 'r':
                cpu_dump(cpu, 0x1);
                break;
            case 'd':
                cpu_dump(cpu, 0xFF);
                break;
            case 'p':
                printf("[0x%08x]  mcause=0x%08x  mepc=0x%08x  mstatus=0x%08x\n",
                       cpu->pc, cpu->mcause, cpu->mepc, cpu->mstatus);
                printf("  mip=0x%08x  mie=0x%08x\n",
                       cpu->mip, cpu->mie);
                printf("sp:0x%08x a0:0x%08x a1:0x%08x a2:0x%08x\n",
                        cpu->sp, cpu->a0, cpu->a1, cpu->a2);
                /* 显示 CLINT 定时器值 */
                {
                    Peripheral *tmr = cpu->peripherals[1];  /* timer at index 1 */
                    uint64_t mt = timer_get_mtime(tmr);
                    uint64_t mc = timer_get_mtimecmp(tmr);
                    printf("  mtime=0x%08x%08x  mtimecmp=0x%08x%08x\n",
                           (uint32_t)(mt >> 32), (uint32_t)mt,
                           (uint32_t)(mc >> 32), (uint32_t)mc);
                }
                break;
            case 'e':
                cpu->dbg_cmd_mode = 1;
                cpu->dbg_buf[0] = 'e';
                cpu->dbg_pos = 1;
                memset(cpu->dbg_buf + 1, 0, sizeof(cpu->dbg_buf) - 1);
                printf("\ne");
                fflush(stdout);
                break;
            case 'q':
                cpu->quit = 1;
                printf("[退出]\n");
                break;
        }
        return;
    }

    /* UART 模式：键盘 → 串口 */
    if (!cpu->halted) {
        uart_inject_char(cpu->peripherals[0], (uint8_t)c, cpu);
    }
}

/* ==================== CPU 主循环 ==================== */

/**
 * cpu_run - 运行 CPU
 * @cpu: CPU 状态结构体指针
 * @max_cycles: 最大执行周期数
 *
 * 取指 - 译码 - 执行 - 写回循环
 */
void cpu_run(CPU *cpu) {
    int cycles = 0;

    printf("========================================\n");
    printf("  RISC-V RV32IMACFC 模拟器\n");
    printf("  Ctrl+] 切换 Monitor/UART 模式\n");
    printf("========================================\n\n");
    printf("开始执行 (PC=0x%08x, RA=0x%08x)...\n\n", cpu->pc, cpu->ra);

    while (!cpu->quit) {
        /* 1. 统一键盘处理（非阻塞） */
        handle_host_input(cpu);

        if(cpu->halted)
            continue;

        /* 2. 中断检查（halted 时跳过，调试断点不响应中断） */
        if (cpu_check_interrupts(cpu))
            continue;

        /* 3. 取指（halted 时跳过） */
        int instr_len = 0;
        uint32_t pc_before_fetch = cpu->pc;
        uint32_t instr = cpu_fetch(cpu, &instr_len);
        if (instr == 0 && instr_len == 0) {
            if (cpu->pc != pc_before_fetch)
                continue;   /* 取指 fault → pc 已指向陷阱向量，进处理器 */
            printf("[CPU] 遇到零指令，停止执行\n");
            break;
        }

        /* 打印当前指令 (调试用) */
#ifdef DEBUG_TRACE
        printf("\r\n[0x%08x] 0x%08x (len=%d)", cpu->pc, instr, instr_len);
#endif

#ifndef DEBUG_TRACE
        if (cpu->step)
            printf("\r\n[0x%08x] 0x%08x (len=%d)", cpu->pc, instr, instr_len);
#endif

        /* 4. 断点检测 */
        cpu_halt_det(cpu, 0, 0);

        /* 5. 执行 */
        int result = cpu_execute(cpu, instr, instr_len);
        if (result <= 0) {
            break;
        }

        /* 6. 外设 tick (定时器更新、中断检查) */
        cpu->cycles = ++cycles;
        for (int i = 0; i < cpu->num_peripherals; i++)
            cpu->peripherals[i]->tick(cpu->peripherals[i], cpu);

        cpu_trace(cpu);

        /* 7. 单步后自动暂停 */
        if (cpu->step) {
            cpu->halted = 1;
            cpu->step = 0;
        }
        if (cpu->halted)
            cpu_dump(cpu, 0x1);
    }

    printf("\n========================================\n");
    printf("  完成 (%d 周期)\n", cycles);
    printf("========================================\n");

    cpu->cycles = cycles;

    /* 打印寄存器状态 */
    cpu_dump(cpu, 0xFF);
}

/**
 * cpu_dump - 打印 CPU 状态
 * @cpu: CPU 状态结构体指针
 */
void cpu_dump(CPU *cpu, int dump_type) {
    if(dump_type&0x1)
    {
        printf("\n=== 通用寄存器 ===\n");
        printf("    pc        ra        sp         gp        tp\n");
        printf("    %08x %08x %08x %08x %08x\n", cpu->pc, cpu->ra, cpu->sp, cpu->gp, cpu->tp);
        printf("    0        1        2        3        4        5        6        7\n");
        printf("s   %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x\n", cpu->s0, cpu->s1, cpu->s2, cpu->s3, cpu->s4, cpu->s5, cpu->s6, cpu->s7,cpu->s8, cpu->s9, cpu->s10, cpu->s11);
        printf("a   %08x %08x %08x %08x %08x %08x %08x %08x\n", cpu->a0, cpu->a1, cpu->a2, cpu->a3, cpu->a4, cpu->a5, cpu->a6, cpu->a7);
        printf("t   %08x %08x %08x %08x %08x %08x %08x \n",     cpu->t0, cpu->t1, cpu->t2, cpu->t3, cpu->t4, cpu->t5, cpu->t6);
    }
    if(dump_type&0x2)
    {
        printf("\n=== 浮点寄存器 ===\n");
        printf("s   %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x\n",cpu->fs0.u, cpu->fs1.u, cpu->fs2.u, cpu->fs3.u, cpu->fs4.u, cpu->fs5.u, cpu->fs6.u, cpu->fs7.u, cpu->fs8.u, cpu->fs9.u, cpu->fs10.u, cpu->fs11.u);
        printf("a   %08x %08x %08x %08x %08x %08x %08x %08x\n", cpu->fa0.u, cpu->fa1.u, cpu->fa2.u, cpu->fa3.u, cpu->fa4.u, cpu->fa5.u, cpu->fa6.u, cpu->fa7.u);
        printf("t   %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x\n",cpu->ft0.u, cpu->ft1.u, cpu->ft2.u, cpu->ft3.u, cpu->ft4.u, cpu->ft5.u, cpu->ft6.u, cpu->ft7.u, cpu->ft8.u, cpu->ft9.u, cpu->ft10.u, cpu->ft11.u, cpu->ft12.u, cpu->ft13.u, cpu->ft14.u, cpu->ft15.u);
    }
    if(dump_type&0x4)
    {
        printf("\n=== 状态 ===\n");
        printf("  PC    = 0x%08x\n", cpu->pc);
        printf("  FCSR  = 0x%08x\n", cpu->fcsr);
        printf("  期数 = %u\n", (uint32_t)cpu->cycles);
    }

    if(dump_type&0x8)
    {
        printf("\n=== 机器模式 CSR ===\n");
        printf("  privilege= %d (0=U 1=S 3=M)\n", cpu->privilege);
        printf("  mstatus  = 0x%08x\n", cpu->mstatus);
        printf("  mtvec    = 0x%08x\n", cpu->mtvec);
        printf("  mepc     = 0x%08x\n", cpu->mepc);
        printf("  mcause   = 0x%08x\n", cpu->mcause);
        printf("  mtval    = 0x%08x\n", cpu->mtval);
        printf("  mie      = 0x%08x\n", cpu->mie);
        printf("  mip      = 0x%08x\n", cpu->mip);
        printf("  mscratch = 0x%08x\n", cpu->mscratch);
        printf("  medeleg  = 0x%08x\n", cpu->medeleg);
        printf("  mideleg  = 0x%08x\n", cpu->mideleg);
        if (cpu->has_reservation) {
            printf("  保留地址 = 0x%08x\n", cpu->reservation_addr);
        }
        printf("\n=== S-mode CSR ===\n");
        printf("  stvec    = 0x%08x\n", cpu->stvec);
        printf("  sepc     = 0x%08x\n", cpu->sepc);
        printf("  scause   = 0x%08x\n", cpu->scause);
        printf("  stval    = 0x%08x\n", cpu->stval);
        printf("  sscratch = 0x%08x\n", cpu->sscratch);
        printf("  sie      = 0x%08x\n", cpu->sie);
        printf("  sip      = 0x%08x\n", cpu->sip);
        printf("  satp     = 0x%08x\n", cpu->satp);
    }
    if(dump_type&0x10)
    {
        uint8_t n = 0;
        printf("\n=== trace  ===\n");
        for(uint32_t i=cpu->trace_idx;i<32;i++)
        {
            printf("%08x ",cpu->trace_buf[i]);
            if(++n%8==0)
                printf("\n");
        }
        n = 0;
        printf("\n");
        for(uint32_t i=0;i<cpu->trace_idx;i++)
        {
            printf("%08x ",cpu->trace_buf[i]);
            if(++n%8==0)
                printf("\n");
        }
        printf("\n");
    }
}

/**
 * cpu_step - 单步执行
 * @cpu: CPU 状态结构体指针
 *
 * 返回：1 表示继续，0 表示停止
 */
int cpu_step(CPU *cpu) {
    if (cpu->halted) {
        return 0;
    }

    int instr_len = 0;
    uint32_t instr = cpu_fetch(cpu, &instr_len);
    if (instr == 0 && instr_len == 0) {
        return 0;
    }

    printf("[0x%08x] 0x%08x (len=%d)\n", cpu->pc, instr, instr_len);

    int result = cpu_execute(cpu, instr, instr_len);
    return result > 0;
}