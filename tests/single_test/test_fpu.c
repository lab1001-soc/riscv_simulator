/*
 * 测试 F 扩展融合乘加指令
 * 编译：gcc -o test_fpu test_fpu.c -lm
 */
#include <stdio.h>
#include <stdint.h>
#include <math.h>

// FMADD.S rd, rs1, rs2, rs3  : rd = rs1 * rs2 + rs3
// 格式：opcode(7) | rd(5) | rs1(5) | rs2(5) | rs3(5) | funct7(7)
// opcode = 0x43, funct7 低 5 位是 rm (舍入模式), 高 2 位是 fmt (00=单精度)

uint32_t make_fmadd(int rd, int rs1, int rs2, int rs3, int rm) {
    // opcode(7) | rd(5) | rm(3) | rs1(5) | rs2(5) | fmt(2) | rs3(5)
    return (0x43 << 0) | (rd << 7) | ((rm & 0x7) << 12) | (rs1 << 15) | (rs2 << 20) | (rs3 << 27);
}

// FMSUB.S: rd = rs1 * rs2 - rs3, opcode = 0x47
uint32_t make_fmsub(int rd, int rs1, int rs2, int rs3, int rm) {
    return (0x47 << 0) | (rd << 7) | ((rm & 0x7) << 12) | (rs1 << 15) | (rs2 << 20) | (rs3 << 27);
}

// FNMSUB.S: rd = -(rs1 * rs2 - rs3), opcode = 0x4B
uint32_t make_fnmsub(int rd, int rs1, int rs2, int rs3, int rm) {
    return (0x4B << 0) | (rd << 7) | ((rm & 0x7) << 12) | (rs1 << 15) | (rs2 << 20) | (rs3 << 27);
}

// FNMADD.S: rd = -(rs1 * rs2 + rs3), opcode = 0x4F
uint32_t make_fnmadd(int rd, int rs1, int rs2, int rs3, int rm) {
    return (0x4F << 0) | (rd << 7) | ((rm & 0x7) << 12) | (rs1 << 15) | (rs2 << 20) | (rs3 << 27);
}

// FLW fd, offset(rs1): 加载浮点数
uint32_t make_flw(int fd, int rs1, int imm) {
    return (0x07 << 0) | (fd << 7) | (rs1 << 15) | ((imm & 0x1F) << 20) | ((imm >> 5) << 25);
}

// FSW fs2, offset(rs1): 存储浮点数
uint32_t make_fsw(int fs2, int rs1, int imm) {
    return (0x27 << 0) | ((imm & 0x1F) << 7) | (rs1 << 15) | (fs2 << 20) | ((imm >> 5) << 25);
}

// FMV.S.X fd, rs1: 将整数移动到浮点寄存器
uint32_t make_fmv_s_x(int fd, int rs1) {
    return (0x53 << 0) | (fd << 7) | (rs1 << 15) | (0 << 20) | (0x780 << 25);
}

// FCVT.S.W fd, rs1: 整数转浮点
uint32_t make_fcvt_s_w(int fd, int rs1, int rm) {
    return (0x53 << 0) | (fd << 7) | (rs1 << 15) | ((rm & 0x7) << 20) | (0x60 << 25);
}

int main() {
    uint32_t prog[100];
    int i = 0;
    
    // 设置 x1 = 0x40400000 (3.0 的 IEEE754 表示)
    prog[i++] = 0x00400093;  // addi x1, x0, 4
    prog[i++] = 0x04009093;  // addi x1, x1, 64 (0x40400000 >> 24 = 4, 但我们需要完整值)
    
    // 更好的方法：直接构造 3.0 和 2.0 的 IEEE754 表示
    // 3.0 = 0x40400000, 2.0 = 0x40000000, 1.0 = 0x3F800000
    prog[i++] = 0x00300113;  // addi x2, x0, 3
    prog[i++] = 0x00200193;  // addi x3, x0, 2
    prog[i++] = 0x00100213;  // addi x4, x0, 1
    
    // 将整数转换为浮点 (使用 FCVT.S.W)
    prog[i++] = make_fcvt_s_w(0, 2, 0);  // fcvt.s.w f0, x2 (f0 = 3.0)
    prog[i++] = make_fcvt_s_w(1, 3, 0);  // fcvt.s.w f1, x3 (f1 = 2.0)
    prog[i++] = make_fcvt_s_w(2, 4, 0);  // fcvt.s.w f2, x4 (f2 = 1.0)
    
    // 测试 FMADD: f3 = f0 * f1 + f2 = 3.0 * 2.0 + 1.0 = 7.0
    prog[i++] = make_fmadd(3, 0, 1, 2, 0);  // fmadd.s f3, f0, f1, f2
    
    // 测试 FMSUB: f4 = f0 * f1 - f2 = 3.0 * 2.0 - 1.0 = 5.0
    prog[i++] = make_fmsub(4, 0, 1, 2, 0);  // fmsub.s f4, f0, f1, f2
    
    // 测试 FNMSUB: f5 = -(f0 * f1 - f2) = -(3.0 * 2.0 - 1.0) = -5.0
    prog[i++] = make_fnmsub(5, 0, 1, 2, 0);  // fnmsub.s f5, f0, f1, f2
    
    // 测试 FNMADD: f6 = -(f0 * f1 + f2) = -(3.0 * 2.0 + 1.0) = -7.0
    prog[i++] = make_fnmadd(6, 0, 1, 2, 0);  // fnmadd.s f6, f0, f1, f2
    
    // ECALL 结束
    prog[i++] = 0x00000073;  // ecall
    
    // 写入二进制文件
    FILE *f = fopen("test_fpu.bin", "wb");
    fwrite(prog, sizeof(uint32_t), i, f);
    fclose(f);
    
    printf("生成测试程序：test_fpu.bin (%d 条指令)\n", i);
    printf("\n预期结果:\n");
    printf("  f0 = 3.0\n");
    printf("  f1 = 2.0\n");
    printf("  f2 = 1.0\n");
    printf("  f3 = 7.0  (FMADD: 3*2+1)\n");
    printf("  f4 = 5.0  (FMSUB: 3*2-1)\n");
    printf("  f5 = -5.0 (FNMSUB: -(3*2-1))\n");
    printf("  f6 = -7.0 (FNMADD: -(3*2+1))\n");
    
    return 0;
}
