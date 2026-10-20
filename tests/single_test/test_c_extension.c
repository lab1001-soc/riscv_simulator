/*
 * test_c_extension.c - C 扩展（压缩指令）测试
 *
 * 压缩指令编码参考：
 * C.ADDI (CI): bits[15:13]=000, bits[12]=imm[5], bits[11:7]=rd, bits[6:2]=imm[4:0], bits[1:0]=01
 * C.LI (CI):   bits[15:13]=010, bits[12]=imm[5], bits[11:7]=rd, bits[6:2]=imm[4:0], bits[1:0]=01
 * C.ADD (CR):  bits[15:13]=100, bits[12]=0, bits[11:7]=rd, bits[6:2]=rs2, bits[1:0]=10
 * C.SLLI (CI): bits[15:13]=000, bits[12]=shamt[5], bits[11:7]=rd, bits[6:2]=shamt[4:0], bits[1:0]=10
 * C.NOP:       0x0001
 * C.EBREAK:    0x9002
 */

#include <stdio.h>
#include <stdint.h>

/* 手动编码压缩指令 */
/* C.ADDI x1, 5: imm[5]=0, rd=1, imm[4:0]=5 */
#define C_ADDI_X1_5    ((0<<13) | (0<<12) | (1<<7) | (5<<2) | 1)
/* C.LI x2, 10: imm[5]=0, rd=2, imm[4:0]=10 */
#define C_LI_X2_10     ((2<<13) | (0<<12) | (2<<7) | (10<<2) | 1)
/* C.SLLI x2, 2: shamt[5]=0, rd=2, shamt[4:0]=2 */
#define C_SLLI_X2_2    ((0<<13) | (0<<12) | (2<<7) | (2<<2) | 2)
/* C.ADD x1, x2: rd=1, rs2=2, bit12=1 for C.ADD */
#define C_ADD_X1_X2    ((4<<13) | (1<<12) | (1<<7) | (2<<2) | 2)
/* C.EBREAK: funct3=4 (bits[15:13]=100), bit12=1, rd=0, rs2=0 */
#define C_EBREAK       0x9002

/* 测试程序 */
static const uint16_t test_program[] = {
    /* C.ADDI x1, 5 - x1 从 0 变为 5 */
    C_ADDI_X1_5,     /* 0x0085 */

    /* C.LI x2, 10 - x2 = 10 */
    C_LI_X2_10,      /* 0x0414 */

    /* C.SLLI x2, 2 - x2 = 10 << 2 = 40 */
    C_SLLI_X2_2,     /* 0x0814 */

    /* C.ADD x1, x2 - x1 = 5 + 40 = 45 */
    C_ADD_X1_X2,     /* 0x9082 */

    /* EBREAK 结束 */
    C_EBREAK,        /* 0x9002 */
};

int main() {
    printf("=== C 扩展（压缩指令）测试 ===\n\n");

    /* 打印编码值 */
    printf("编码值:\n");
    printf("  C.ADDI x1, 5  = 0x%04x\n", C_ADDI_X1_5);
    printf("  C.LI x2, 10   = 0x%04x\n", C_LI_X2_10);
    printf("  C.SLLI x2, 2  = 0x%04x\n", C_SLLI_X2_2);
    printf("  C.ADD x1, x2  = 0x%04x\n", C_ADD_X1_X2);
    printf("  C.EBREAK      = 0x%04x\n", C_EBREAK);

    printf("\n测试程序：%d 条 16 位指令\n\n", (int)(sizeof(test_program) / 2));

    /* 输出二进制文件 */
    FILE *f = fopen("test_c.bin", "wb");
    if (!f) {
        perror("无法创建文件");
        return 1;
    }
    fwrite(test_program, sizeof(uint16_t), sizeof(test_program) / 2, f);
    fclose(f);

    printf("已生成：test_c.bin\n\n");
    printf("预期结果:\n");
    printf("  x1 = 45        (C.ADDI 5, C.ADD +40)\n");
    printf("  x2 = 40        (C.LI 10, C.SLLI <<2)\n");
    printf("\n运行：./rv_sim test_c.bin\n");

    return 0;
}