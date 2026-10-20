/*
 * test_f_extension.c - F 扩展 (浮点) 指令测试
 * 编译：gcc -o test_f test_f_extension.c -lm && ./test_f
 */

#include <stdio.h>
#include <stdint.h>
#include <math.h>

/* 测试程序 */
static const uint32_t test_program[] = {
    /* 准备数据：整数转浮点 */
    0x00300113,  /* addi  x2, x0, 3        ; x2 = 3 */
    0x00200193,  /* addi  x3, x0, 2        ; x3 = 2 */
    0x00100213,  /* addi  x4, x0, 1        ; x4 = 1 */
    0xC0010053,  /* fcvt.s.w f0, x2        ; f0 = 3.0 */
    0xC00180D3,  /* fcvt.s.w f1, x3        ; f1 = 2.0 */
    0xC0020153,  /* fcvt.s.w f2, x4        ; f2 = 1.0 */
    
    /* ========== 融合乘加指令 (核心测试) ========== */
    0x101001C3,  /* fmadd.s f3, f0, f1, f2  ; f3 = 3*2 + 1 = 7.0 */
    0x10100247,  /* fmsub.s f4, f0, f1, f2  ; f4 = 3*2 - 1 = 5.0 */
    0x101002CB,  /* fnmsub.s f5, f0, f1, f2 ; f5 = -(3*2 - 1) = -5.0 */
    0x1010034F,  /* fnmadd.s f6, f0, f1, f2 ; f6 = -(3*2 + 1) = -7.0 */
    
    /* ========== 基础浮点运算 ========== */
    0x00205203,  /* fadd.s f4, f0, f2       ; f4 = 3.0 + 1.0 = 4.0 */
    0x00205283,  /* fsub.s f5, f0, f2       ; f5 = 3.0 - 1.0 = 2.0 */
    0x00202303,  /* fmul.s f6, f0, f1       ; f6 = 3.0 * 2.0 = 6.0 */
    0x00202383,  /* fdiv.s f7, f0, f1       ; f7 = 3.0 / 2.0 = 1.5 */
    0x00102403,  /* fsqrt.s f8, f2          ; f8 = sqrt(1.0) = 1.0 */
    
    /* ========== 浮点比较 ========== */
    0x00205553,  /* fle.s x10, f0, f1       ; x10 = (3.0 <= 2.0) = 0 */
    0x002055D3,  /* flt.s x11, f0, f1       ; x11 = (3.0 < 2.0) = 0 */
    0x00202653,  /* feq.s x12, f0, f1       ; x12 = (3.0 == 2.0) = 0 */
    0x001026D3,  /* feq.s x13, f1, f2       ; x13 = (2.0 == 1.0) = 0 */
    0x0020A753,  /* fle.s x14, f1, f0       ; x14 = (2.0 <= 3.0) = 1 */
    
    /* ========== 浮点最值 ========== */
    0x00205783,  /* fmin.s f15, f0, f1      ; f15 = min(3.0, 2.0) = 2.0 */
    0x00205803,  /* fmax.s f16, f0, f1      ; f16 = max(3.0, 2.0) = 3.0 */
    
    /* ECALL 结束 */
    0x00000073,  /* ecall */
};

int main() {
    printf("=== F 扩展 (浮点) 指令测试 ===\n\n");
    printf("测试程序：%d 条指令\n\n", (int)(sizeof(test_program) / 4));
    
    /* 输出二进制文件 */
    FILE *f = fopen("test_f.bin", "wb");
    if (!f) {
        perror("无法创建文件");
        return 1;
    }
    fwrite(test_program, sizeof(uint32_t), sizeof(test_program) / 4, f);
    fclose(f);
    
    printf("已生成：test_f.bin\n\n");
    printf("预期结果:\n");
    printf("  --- 融合乘加 ---\n");
    printf("  f3 =  7.0       (FMADD: 3*2+1)\n");
    printf("  f4 =  5.0       (FMSUB: 3*2-1)\n");
    printf("  f5 = -5.0       (FNMSUB: -(3*2-1))\n");
    printf("  f6 = -7.0       (FNMADD: -(3*2+1))\n");
    printf("  --- 基础运算 ---\n");
    printf("  f4 =  4.0       (FADD: 3+1)\n");
    printf("  f5 =  2.0       (FSUB: 3-1)\n");
    printf("  f6 =  6.0       (FMUL: 3*2)\n");
    printf("  f7 =  1.5       (FDIV: 3/2)\n");
    printf("  f8 =  1.0       (FSQRT: sqrt(1))\n");
    printf("  --- 比较 ---\n");
    printf("  x10 = 0         (FLE: 3<=2)\n");
    printf("  x11 = 0         (FLT: 3<2)\n");
    printf("  x12 = 0         (FEQ: 3==2)\n");
    printf("  x14 = 1         (FLE: 2<=3)\n");
    printf("  --- 最值 ---\n");
    printf("  f15 = 2.0       (FMIN)\n");
    printf("  f16 = 3.0       (FMAX)\n");
    printf("\n运行：./rv_sim test_f.bin\n");
    
    return 0;
}
