/*
 * test_base_integer.c - 基础整数指令测试 (I 扩展)
 * 编译：gcc -o test_base test_base_integer.c && ./test_base
 */

#include <stdio.h>
#include <stdint.h>

/* 测试程序 */
static const uint32_t test_program[] = {
    /* ========== 算术运算 ========== */
    0x00A00113,  /* addi  x2, x0, 10       ; x2 = 10 */
    0x00B00193,  /* addi  x3, x0, 11       ; x3 = 11 */
    0x00310233,  /* add   x4, x2, x3       ; x4 = 10 + 11 = 21 */
    0x403102B3,  /* sub   x5, x2, x3       ; x5 = 10 - 11 = -1 */
    
    /* ========== 逻辑运算 ========== */
    0x00314333,  /* xor   x6, x2, x3       ; x6 = 10 ^ 11 = 1 */
    0x003163B3,  /* or    x7, x2, x3       ; x7 = 10 | 11 = 11 */
    0x00317433,  /* and   x8, x2, x3       ; x8 = 10 & 11 = 10 */
    
    /* ========== 移位运算 ========== */
    0x00200113,  /* addi  x2, x0, 2        ; x2 = 2 */
    0x003114B3,  /* sll   x9, x2, x3       ; x9 = 2 << 11 = 4096 */
    0x00311533,  /* srl   x10, x2, x3      ; x10 = 2 >> 11 = 0 */
    0x403115B3,  /* sra   x11, x2, x3      ; x11 = 2 >> 11 (算术) = 0 */
    
    /* ========== 比较运算 ========== */
    0x00A00113,  /* addi  x2, x0, 10       ; x2 = 10 */
    0x00B00193,  /* addi  x3, x0, 11       ; x3 = 11 */
    0x00312633,  /* slt   x12, x2, x3      ; x12 = (10 < 11) = 1 */
    0x003136B3,  /* sltu  x13, x2, x3      ; x13 = (10u < 11u) = 1 */
    
    /* ========== 立即数运算 ========== */
    0xFFE00113,  /* addi  x2, x0, -2       ; x2 = -2 */
    0xF0010793,  /* addi  x15, x2, -256    ; x15 = -2 + (-256) = -258 */
    0x0FF12813,  /* andi  x16, x2, 0xFF    ; x16 = -2 & 0xFF = 254 */
    0x00F12893,  /* xori  x17, x2, 0xF     ; x17 = -2 ^ 0xF = -3 */
    
    /* ========== 上位立即数 ========== */
    0x12345337,  /* lui   x18, 0x12345     ; x18 = 0x12345000 */
    0x12345317,  /* auipc x19, 0x12345     ; x19 = PC + 0x12345000 */
    
    /* ========== 分支跳转 ========== */
    0x00000893,  /* addi  x17, x0, 0       ; x17 = 0 */
    0x0040006F,  /* j     skip1            ; 跳转 */
    0x06400893,  /* addi  x17, x0, 100     ; 不会执行 */
    /* skip1: */
    0x00F00913,  /* addi  x18, x0, 15      ; x18 = 15 */
    
    /* ========== 条件分支 ========== */
    0x00A00A13,  /* addi  x20, x0, 10      ; x20 = 10 */
    0x00A00A93,  /* addi  x21, x0, 10      ; x21 = 10 */
    0x01500063,  /* beq   x20, x21, eq     ; 相等则跳转 */
    0x00000B13,  /* addi  x22, x0, 0       ; 不相等时执行 (不会执行) */
    /* eq: */
    0x00100B13,  /* addi  x22, x0, 1       ; x22 = 1 (相等标志) */
    
    /* ECALL 结束 */
    0x00000073,  /* ecall */
};

int main() {
    printf("=== 基础整数指令测试 (I 扩展) ===\n\n");
    printf("测试程序：%d 条指令\n\n", (int)(sizeof(test_program) / 4));
    
    /* 输出二进制文件 */
    FILE *f = fopen("test_base.bin", "wb");
    if (!f) {
        perror("无法创建文件");
        return 1;
    }
    fwrite(test_program, sizeof(uint32_t), sizeof(test_program) / 4, f);
    fclose(f);
    
    printf("已生成：test_base.bin\n\n");
    printf("预期结果:\n");
    printf("  --- 算术 ---\n");
    printf("  x4  = 21        (ADD: 10+11)\n");
    printf("  x5  = -1        (SUB: 10-11)\n");
    printf("  --- 逻辑 ---\n");
    printf("  x6  = 1         (XOR: 10^11)\n");
    printf("  x7  = 11        (OR: 10|11)\n");
    printf("  x8  = 10        (AND: 10&11)\n");
    printf("  --- 移位 ---\n");
    printf("  x9  = 4096      (SLL: 2<<11)\n");
    printf("  x10 = 0         (SRL: 2>>11)\n");
    printf("  --- 比较 ---\n");
    printf("  x12 = 1         (SLT: 10<11)\n");
    printf("  x13 = 1         (SLTU)\n");
    printf("  --- 立即数 ---\n");
    printf("  x15 = -258      (ADDI)\n");
    printf("  x16 = 254       (ANDI)\n");
    printf("  x17 = -3        (XORI)\n");
    printf("  --- 上位立即数 ---\n");
    printf("  x18 = 0x12345000 (LUI)\n");
    printf("  x19 = PC+0x12345000 (AUIPC)\n");
    printf("  --- 跳转 ---\n");
    printf("  x18 = 15        (J 跳转成功)\n");
    printf("  x22 = 1         (BEQ 相等跳转成功)\n");
    printf("\n运行：./rv_sim test_base.bin\n");
    
    return 0;
}
