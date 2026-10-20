/*
 * test_simple_fix.c - 验证 CSR 和基本指令修复
 */

#include <stdio.h>
#include <stdint.h>

/* 简单测试验证核心修复 */
static const uint32_t test_program[] = {
    /* M 扩展测试 */
    0x00500113,  /* addi  x2, x0, 5        ; x2 = 5 */
    0x00700193,  /* addi  x3, x0, 7        ; x3 = 7 */
    0x023100B3,  /* mul   x1, x2, x3       ; x1 = 35 */

    /* 基础整数测试 */
    0x00A00113,  /* addi  x2, x0, 10       ; x2 = 10 */
    0x00B00193,  /* addi  x3, x0, 11       ; x3 = 11 */
    0x00310233,  /* add   x4, x2, x3       ; x4 = 21 */
    0x403102B3,  /* sub   x5, x2, x3       ; x5 = -1 */

    /* CSR 测试 - 验证不再误判为 ECALL */
    0x00100913,  /* addi  x18, x0, 1       ; x18 = 1 */
    0x0030A973,  /* csrrs x19, fflags, x18 ; 读并置位 FFLAGS */

    /* F 扩展 - FCVT.S.W (正确编码) */
    0x00300113,  /* addi  x2, x0, 3        ; x2 = 3 */
    0x38001053,  /* fcvt.s.w f0, x2        ; f0 = 3.0 */

    /* F 扩展 - FADD.S */
    0x00005053,  /* fadd.s f0, f0, f0      ; f0 = 3.0 + 3.0 = 6.0 */

    /* ECALL 结束 */
    0x00000073,  /* ecall */
};

int main() {
    printf("=== 简单验证测试 ===\n\n");

    FILE *f = fopen("test_simple.bin", "wb");
    fwrite(test_program, sizeof(uint32_t), sizeof(test_program) / 4, f);
    fclose(f);

    printf("已生成：test_simple.bin\n");
    printf("预期：x1=35, x4=21, x5=-1, f0=6.0\n\n");
    printf("运行：./rv_sim test_simple.bin\n");

    return 0;
}