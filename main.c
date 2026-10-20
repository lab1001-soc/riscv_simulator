/*
 * main.c - RISC-V RV32IMACF 模拟器主程序
 *
 * 编译：gcc -o rv_sim main.c cpu.c fetch.c decode.c execute.c writeback.c -lm
 */

#include "cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif

/* 函数声明 */
int cpu_load_program(CPU *cpu, const uint32_t *program, size_t size);
int cpu_load_binary(CPU *cpu, const char *filename);

/* 测试程序 - RV32IMACF 指令集测试 */
static const uint32_t test_program[] = {
    /* ========== M 扩展测试 (乘除) ========== */
    0x00500113,  /* addi  x2, x0, 5        ; x2 = 5 */
    0x00700193,  /* addi  x3, x0, 7        ; x3 = 7 */
    0x023100B3,  /* mul   x1, x2, x3       ; x1 = 5 * 7 = 35 */
    0x02314133,  /* div   x2, x2, x3       ; x2 = 5 / 7 = 0 */
    0x023161B3,  /* rem   x3, x2, x3       ; x3 = 0 % 7 = 0 */
    0x00800293,  /* addi  x5, x0, 8        ; x5 = 8 */
    0x00300313,  /* addi  x6, x0, 3        ; x6 = 3 */
    0x0262D3B3,  /* divu  x7, x5, x6       ; x7 = 8 / 3 = 2 */
    0x0262F433,  /* remu  x8, x5, x6       ; x8 = 8 % 3 = 2 */

    /* ========== F 扩展测试 (浮点运算) ========== */
    0x00300113,  /* addi  x2, x0, 3        ; x2 = 3 */
    0x00200193,  /* addi  x3, x0, 2        ; x3 = 2 */
    0x00100213,  /* addi  x4, x0, 1        ; x4 = 1 */

    /* 整数转浮点 (FCVT.S.W: funct7=0x38, rs2=0) */
    0x38001053,  /* fcvt.s.w f0, x2        ; f0 = 3.0 */
    0x380018D3,  /* fcvt.s.w f1, x3        ; f1 = 2.0 */
    0x38002153,  /* fcvt.s.w f2, x4        ; f2 = 1.0 */

    /* 融合乘加指令测试 (FMADD: opcode=0x43) */
    0x101001C3,  /* fmadd.s f3, f0, f1, f2 ; f3 = f0*f1 + f2 = 3*2+1 = 7.0 */
    0x10100247,  /* fmsub.s f4, f0, f1, f2 ; f4 = f0*f1 - f2 = 3*2-1 = 5.0 */
    0x101002CB,  /* fnmsub.s f5, f0, f1, f2; f5 = -(f0*f1 - f2) = -5.0 */
    0x1010034F,  /* fnmadd.s f6, f0, f1, f2; f6 = -(f0*f1 + f2) = -7.0 */

    /* 基础浮点运算 (FADD: funct5=0x00, FSUB: funct5=0x01, FMUL: funct5=0x02, FDIV: funct5=0x03) */
    0x00005203,  /* fadd.s f4, f0, f2      ; f4 = 3.0 + 1.0 = 4.0 */
    0x04005283,  /* fsub.s f5, f0, f2      ; f5 = 3.0 - 1.0 = 2.0 */
    0x08002303,  /* fmul.s f6, f0, f1      ; f6 = 3.0 * 2.0 = 6.0 */
    0x0C002383,  /* fdiv.s f7, f0, f1      ; f7 = 3.0 / 2.0 = 1.5 */

    /* 浮点比较 (FLE/FLT/FEQ: funct5=0x07) */
    0x1C00A553,  /* fle.s x10, f0, f1      ; x10 = (3.0 <= 2.0) = 0 */
    0x1C00A5D3,  /* flt.s x11, f0, f1      ; x11 = (3.0 < 2.0) = 0 */
    0x1C002653,  /* feq.s x12, f0, f1      ; x12 = (3.0 == 2.0) = 0 */
    0x1C0126D3,  /* feq.s x13, f1, f2      ; x13 = (2.0 == 1.0) = 0 */

    /* ========== 基础整数运算测试 ========== */
    0x00A00793,  /* addi  x15, x0, 10      ; x15 = 10 */
    0x00B00813,  /* addi  x16, x0, 11      ; x16 = 11 */
    0x08F788B3,  /* add   x17, x15, x16    ; x17 = 10 + 11 = 21 */
    0x48F78933,  /* sub   x18, x15, x16    ; x18 = 10 - 11 = -1 */
    0x08F799B3,  /* sll   x19, x15, x16    ; x19 = 10 << 11 = 20480 */
    0x08F7CA33,  /* xor   x20, x15, x16    ; x20 = 10 ^ 11 = 1 */
    0x08F7EAB3,  /* or    x21, x15, x16    ; x21 = 10 | 11 = 11 */
    0x08F7FB33,  /* and   x22, x15, x16    ; x22 = 10 & 11 = 10 */

    /* ========== 分支和跳转测试 ========== */
    0x0040006F,  /* j     skip1            ; 跳转到 skip1 (偏移 4) */
    0x06400B93,  /* addi  x23, x0, 100     ; 这行不会被执行 */
    /* skip1: */
    0x00F00C13,  /* addi  x24, x0, 15      ; x24 = 15 */

    /* ECALL 结束 */
    0x00000073,  /* ecall                  ; 系统调用，结束程序 */
};

static void print_usage(const char *prog) {
    printf("RISC-V RV32IMACF 模拟器\n\n");
    printf("用法：%s [选项] [文件.bin]\n\n", prog);
    printf("选项:\n");
    printf("  -h, --help     显示帮助信息\n");
    printf("  -t, --test     运行内置测试程序\n");
    printf("  -l, --linux    Linux 启动模式\n");
    printf("  -d, --debug    启用调试输出\n");
    printf("\n");
    printf("如果不指定文件，默认运行内置测试程序。\n");
    printf("\n");
    printf("支持的指令集: RV32IMACF\n");
}

int main(int argc, char *argv[]) {
    CPU cpu;
    int run_test = 1;
    int linux_mode = 0;
    const char *filename = NULL;

#ifdef _WIN32
    SetConsoleOutputCP(65001);
#endif

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--test") == 0) {
            run_test = 1;
        } else if (strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--linux") == 0) {
            linux_mode = 1;
            run_test = 0;
        } else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--debug") == 0) {
            printf("[DEBUG] 调试模式已启用\n");
        } else if (argv[i][0] != '-') {
            filename = argv[i];
            run_test = 0;
        } else {
            fprintf(stderr, "未知选项：%s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }

    /* 初始化 CPU */
    cpu_init(&cpu);

    if (linux_mode) {
        /* Linux 启动模式 (PHYS_RAM_BASE=0x80000000):
         *   IMEM 0x1000000: fw_stub.bin (M-mode stub, tiny)
         *   IMEM 0x1001000: rv_sim.dtb (Device Tree)
         *   LMEM 0x80000000: Image (Linux kernel, ~21MB)
         *   LMEM 0x83000000: rootfs.cpio (initramfs)
         */
        printf("=== Linux 启动模式 ===\n");
        printf("加载 fw_stub.bin → 0x1000000\n");
        if (cpu_load_binary_at(&cpu, "tests/linux/fw_stub.bin", 0x1000000) != 0)
            return 1;
        printf("加载 rv_sim.dtb  → 0x1001000\n");
        if (cpu_load_binary_at(&cpu, "tests/linux/rv_sim.dtb", 0x1001000) != 0)
            return 1;
        printf("加载 rootfs.cpio → 0x83000000\n");
        cpu_load_binary_at(&cpu, "tests/linux/rootfs.cpio", 0x83000000);
        printf("加载 Image      → 0x80000000\n");
        if (cpu_load_binary_at(&cpu, "tests/linux/Image", 0x80000000) != 0) {
            printf("[ERROR] Linux Image 未找到\n");
            return 1;
        }

        cpu.pc = 0x1000000;
        cpu.privilege = PRIV_M;
    } else if (run_test || filename == NULL) {
        /* 运行内置测试程序 */
        printf("运行内置测试程序...\n\n");
        cpu_load_program(&cpu, test_program, sizeof(test_program));
    } else {
        /* 从文件加载 */
        printf("从文件加载：%s\n\n", filename);
        if (cpu_load_binary(&cpu, filename) != 0) {
            return 1;
        }
    }

    /* 运行 CPU */
    cpu_run(&cpu);

    return 0;
}
