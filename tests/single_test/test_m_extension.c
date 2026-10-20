/*
 * test_m_extension.c - M 扩展 (乘除) 指令测试
 * 编译：gcc -o test_m test_m_extension.c && ./test_m
 */

#include <stdio.h>
#include <stdint.h>

/* 测试程序 */
static const uint32_t test_program[] = {
    /* MUL 测试 */
    0x00500113,  /* addi  x2, x0, 5        ; x2 = 5 */
    0x00700193,  /* addi  x3, x0, 7        ; x3 = 7 */
    0x023100B3,  /* mul   x1, x2, x3       ; x1 = 5 * 7 = 35 */
    
    /* MULH 测试 (有符号高位乘法) */
    0xFFF00213,  /* addi  x4, x0, -1       ; x4 = -1 */
    0x00400293,  /* addi  x5, x0, 4        ; x5 = 4 */
    0x02525333,  /* mulh  x6, x4, x5       ; x6 = (-1 * 4) >> 32 = -1 */
    
    /* DIV 测试 */
    0x00A00313,  /* addi  x6, x0, 10       ; x6 = 10 */
    0x00300393,  /* addi  x7, x0, 3        ; x7 = 3 */
    0x02735433,  /* div   x8, x6, x7       ; x8 = 10 / 3 = 3 */
    
    /* DIVU 测试 (无符号除法) */
    0x00800413,  /* addi  x8, x0, 8        ; x8 = 8 */
    0x00300493,  /* addi  x9, x0, 3        ; x9 = 3 */
    0x02945533,  /* divu  x10, x8, x9      ; x10 = 8 / 3 = 2 */
    
    /* REM 测试 (有符号取余) */
    0x027375B3,  /* rem   x11, x6, x7      ; x11 = 10 % 3 = 1 */
    
    /* REMU 测试 (无符号取余) */
    0x02947633,  /* remu  x12, x8, x9      ; x12 = 8 % 3 = 2 */
    
    /* ECALL 结束 */
    0x00000073,  /* ecall */
};

int main() {
    printf("=== M 扩展 (乘除) 指令测试 ===\n\n");
    printf("测试程序：%d 条指令\n\n", (int)(sizeof(test_program) / 4));
    
    /* 输出二进制文件 */
    FILE *f = fopen("test_m.bin", "wb");
    if (!f) {
        perror("无法创建文件");
        return 1;
    }
    fwrite(test_program, sizeof(uint32_t), sizeof(test_program) / 4, f);
    fclose(f);
    
    printf("已生成：test_m.bin\n\n");
    printf("预期结果:\n");
    printf("  x1  = 35        (MUL: 5 * 7)\n");
    printf("  x6  = -1        (MULH: -1 * 4 高位)\n");
    printf("  x8  = 3         (DIV: 10 / 3)\n");
    printf("  x10 = 2         (DIVU: 8 / 3)\n");
    printf("  x11 = 1         (REM: 10 %% 3)\n");
    printf("  x12 = 2         (REMU: 8 %% 3)\n");
    printf("\n运行：./rv_sim test_m.bin\n");
    
    return 0;
}
