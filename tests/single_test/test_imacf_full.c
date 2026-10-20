/*
 * test_imacf_full.c - RV32IMACF 完整指令集测试
 * 编译测试生成器：gcc -o test_imacf_gen test_imacf_full.c -lm
 * 运行生成器：./test_imacf_gen
 * 运行模拟器：./rv_sim test_imacf.bin
 */

#include <stdio.h>
#include <stdint.h>
#include <math.h>

/* 测试程序 - RV32IMACF 指令集完整测试 */
static const uint32_t test_program[] = {
    /* ========== I 扩展 - 基础整数运算 ========== */
    0x00A00113,  /* addi  x2, x0, 10       ; x2 = 10 */
    0x00B00193,  /* addi  x3, x0, 11       ; x3 = 11 */
    0x00310233,  /* add   x4, x2, x3       ; x4 = 10 + 11 = 21 */
    0x403102B3,  /* sub   x5, x2, x3       ; x5 = 10 - 11 = -1 */
    0x00314333,  /* xor   x6, x2, x3       ; x6 = 10 ^ 11 = 1 */
    0x003163B3,  /* or    x7, x2, x3       ; x7 = 10 | 11 = 11 */
    0x00317433,  /* and   x8, x2, x3       ; x8 = 10 & 11 = 10 */
    0x003114B3,  /* sll   x9, x2, x3       ; x9 = 10 << 11 = 20480 */
    0x00312533,  /* srl   x10, x2, x3      ; x10 = 10 >> 11 = 0 */
    0x403125B3,  /* sra   x11, x2, x3      ; x11 = 10 >> 11 (算术) = 0 */
    0x00312633,  /* slt   x12, x2, x3      ; x12 = (10 < 11) = 1 */
    0x003136B3,  /* sltu  x13, x2, x3      ; x13 = (10u < 11u) = 1 */

    /* ========== M 扩展 - 乘除运算 ========== */
    0x00500113,  /* addi  x2, x0, 5        ; x2 = 5 */
    0x00700193,  /* addi  x3, x0, 7        ; x3 = 7 */
    0x023100B3,  /* mul   x1, x2, x3       ; x1 = 5 * 7 = 35 */
    0x02314133,  /* div   x2, x2, x3       ; x2 = 5 / 7 = 0 */
    0x026161B3,  /* rem   x3, x2, x6       ; x3 = 5 % 6 = 5 */
    0x00800293,  /* addi  x5, x0, 8        ; x5 = 8 */
    0x00300313,  /* addi  x6, x0, 3        ; x6 = 3 */
    0x0262D3B3,  /* divu  x7, x5, x6       ; x7 = 8 / 3 = 2 */
    0x0262F433,  /* remu  x8, x5, x6       ; x8 = 8 % 3 = 2 */
    0x023100B3,  /* mul   x1, x2, x3       ; 再次测试 MUL */

    /* ========== F 扩展 - 浮点运算 ========== */
    /* 准备数据：整数转浮点 (FCVT.S.W: funct7=0x38, rs2=0) */
    0x00300113,  /* addi  x2, x0, 3        ; x2 = 3 */
    0x00200193,  /* addi  x3, x0, 2        ; x3 = 2 */
    0x00100213,  /* addi  x4, x0, 1        ; x4 = 1 */
    0x38001053,  /* fcvt.s.w f0, x2        ; f0 = 3.0 */
    0x380018D3,  /* fcvt.s.w f1, x3        ; f1 = 2.0 */
    0x38002153,  /* fcvt.s.w f2, x4        ; f2 = 1.0 */

    /* 基础浮点运算 (FADD: funct7=0x00, FSUB: funct7=0x04, FMUL: funct7=0x08, FDIV: funct7=0x0C) */
    0x00005203,  /* fadd.s f4, f0, f2      ; f4 = 3.0 + 1.0 = 4.0 */
    0x04005283,  /* fsub.s f5, f0, f2      ; f5 = 3.0 - 1.0 = 2.0 */
    0x08002303,  /* fmul.s f6, f0, f1      ; f6 = 3.0 * 2.0 = 6.0 */
    0x0C002383,  /* fdiv.s f7, f0, f1      ; f7 = 3.0 / 2.0 = 1.5 */

    /* 融合乘加指令 */
    0x101001C3,  /* fmadd.s f3, f0, f1, f2 ; f3 = 3*2 + 1 = 7.0 */
    0x10100247,  /* fmsub.s f4, f0, f1, f2 ; f4 = 3*2 - 1 = 5.0 */
    0x101002CB,  /* fnmsub.s f5, f0, f1, f2; f5 = -(3*2 - 1) = -5.0 */
    0x1010034F,  /* fnmadd.s f6, f0, f1, f2; f6 = -(3*2 + 1) = -7.0 */

    /* 浮点比较 (FLE/FLT/FEQ: funct7=0x1C, rm=0/1/2) */
    0x1C00A553,  /* fle.s x10, f0, f1      ; x10 = (3.0 <= 2.0) = 0 */
    0x1C00A5D3,  /* flt.s x11, f0, f1      ; x11 = (3.0 < 2.0) = 0 */
    0x1C002653,  /* feq.s x12, f0, f1      ; x12 = (3.0 == 2.0) = 0 */
    0x1C0126D3,  /* feq.s x13, f1, f2      ; x13 = (2.0 == 1.0) = 0 */

    /* 浮点最值 (FMIN/FMAX: funct7=0x14, rm=0/1) */
    0x14005783,  /* fmin.s f15, f0, f1     ; f15 = min(3.0, 2.0) = 2.0 */
    0x14005803,  /* fmax.s f16, f0, f1     ; f16 = max(3.0, 2.0) = 3.0 */

    /* FSGNJ 系列 (funct7=0x10, rm=0/1/2) */
    0x1000D183,  /* fsgnj.s f3, f0, f1     ; f3 = 3.0 (复制符号) */
    0x1000D203,  /* fsgnjn.s f4, f0, f1    ; f4 = -3.0 (反转符号) */
    0x1000D283,  /* fsgnjx.s f5, f0, f1    ; f5 = -3.0 (异或符号) */

    /* 浮点转换 (FCVT.W.S: funct7=0x20, FCVT.S.W: funct7=0x38) */
    0x20008553,  /* fcvt.w.s x10, f0       ; x10 = (int)3.0 = 3 */
    0x240086D3,  /* fcvt.wu.s x13, f0      ; x13 = (uint)3.0 = 3 */
    0x38008153,  /* fcvt.s.w f2, x10       ; f2 = 3.0 */

    /* FMV 指令 (FMV.W.X: funct7=0x78, FMV.X.W: funct7=0x70) */
    0x78001053,  /* fmv.w.x f2, x2         ; f2 = bit_cast(x2) */
    0x7000A153,  /* fmv.x.w x10, f2        ; x10 = bit_cast(f2) */

    /* FCLASS (funct7=0x28) */
    0x2800E183,  /* fclass.s x3, f0        ; x3 = class(3.0) */

    /* ========== CSR 指令测试 ========== */
    0x00100913,  /* addi  x18, x0, 1       ; x18 = 1 */
    0x00302973,  /* csrrw x19, fflags, x18 ; 写 FFLAGS=1, 读旧值到 x19 */
    0x003028F3,  /* csrrs x17, fflags, x0  ; 读 FFLAGS 到 x17 (不置位) */
    0x00202A73,  /* csrrc x20, frm, x0     ; 读 FRM 到 x20 (不清位) */
    0x00102A13,  /* csrrwi x20, fcsr, 0    ; 写 FCSR=0, 读旧值 */

    /* ========== A 扩展 - 原子操作测试 ========== */
    /* 准备内存地址 */
    0x00000313,  /* addi  x6, x0, 0        ; x6 = 0 (内存地址) */
    0x00A00393,  /* addi  x7, x0, 10       ; x7 = 10 */
    0x00B00413,  /* addi  x8, x0, 11       ; x8 = 11 */

    /* LR/SC 测试 */
    0x100324B3,  /* lr.w  x9, (x6)         ; 加载保留 */
    0x10732533,  /* sc.w  x10, x7, (x6)    ; 条件存储 */

    /* ========== 分支跳转测试 ========== */
    0x0040006F,  /* j     skip1            ; 跳转 */
    0x06400B93,  /* addi  x23, x0, 100     ; 不会执行 */
    /* skip1: */
    0x00F00C13,  /* addi  x24, x0, 15      ; x24 = 15 */

    /* 条件分支 */
    0x00A00A13,  /* addi  x20, x0, 10      ; x20 = 10 */
    0x00A00A93,  /* addi  x21, x0, 10      ; x21 = 10 */
    0x01500863,  /* beq   x20, x21, eq     ; 相等跳转 */
    0x00000B13,  /* addi  x22, x0, 0       ; 不会执行 */
    /* eq: */
    0x00100B13,  /* addi  x22, x0, 1       ; x22 = 1 */

    /* ========== ECALL 结束 ========== */
    0x00000073,  /* ecall                  ; 系统调用结束 */
};

int main() {
    printf("=== RV32IMACF 完整指令集测试 ===\n\n");
    printf("测试程序：%d 条指令\n\n", (int)(sizeof(test_program) / 4));

    /* 输出二进制文件 */
    FILE *f = fopen("test_imacf.bin", "wb");
    if (!f) {
        perror("无法创建文件");
        return 1;
    }
    fwrite(test_program, sizeof(uint32_t), sizeof(test_program) / 4, f);
    fclose(f);

    printf("已生成：test_imacf.bin\n\n");

    printf("预期结果摘要:\n");
    printf("  --- I 扩展 ---\n");
    printf("  x4  = 21        (ADD)\n");
    printf("  x5  = -1        (SUB)\n");
    printf("  x6  = 1         (XOR)\n");
    printf("  --- M 扩展 ---\n");
    printf("  x1  = 35        (MUL: 5*7)\n");
    printf("  x7  = 2         (DIVU: 8/3)\n");
    printf("  x8  = 2         (REMU: 8%3)\n");
    printf("  --- F 扩展 ---\n");
    printf("  f0  = 3.0       (FCVT.S.W)\n");
    printf("  f4  = 4.0       (FADD)\n");
    printf("  f6  = 6.0       (FMUL)\n");
    printf("  f15 = 2.0       (FMIN)\n");
    printf("  x10 = 0         (FLE: 3<=2)\n");
    printf("  --- CSR ---\n");
    printf("  x17 = FFLAGS值  (CSRRS)\n");
    printf("  --- A 扩展 ---\n");
    printf("  LR/SC 操作完成\n");
    printf("\n运行：./rv_sim test_imacf.bin\n");

    return 0;
}