/*
 * test_a_extension.c - A 扩展 (原子操作) 指令测试
 * 编译：gcc -o test_a test_a_extension.c && ./test_a
 */

#include <stdio.h>
#include <stdint.h>

/* 测试程序 */
static const uint32_t test_program[] = {
    /* 初始化内存地址 0x100 为 100 */
    0x06400097,  /* auipc x1, 0x64         ; x1 = PC + 0x64000 */
    0x06408093,  /* addi  x1, x1, 100      ; x1 = 目标地址 */
    0x06400113,  /* addi  x2, x0, 100      ; x2 = 100 (初始值) */
    0x00212023,  /* sw    x2, 0(x1)        ; 内存 [x1] = 100 */
    
    /* LR.W 测试 - 加载保留字 */
    0x000120AF,  /* lr.w  x1, 0(x1)        ; x1 = 内存值，设置保留 */
    
    /* 准备原子操作测试数据 */
    0x00A00293,  /* addi  x5, x0, 10       ; x5 = 10 */
    0x01400313,  /* addi  x6, x0, 20       ; x6 = 20 */
    
    /* AMOADD.W - 原子加法 */
    0x0061212F,  /* amoadd.w x2, x6, (x1)  ; x2 = old_val, mem = old_val + 20 */
    
    /* AMOAND.W - 原子与 */
    0x00C121AF,  /* amoand.w x3, x5, (x1)  ; x3 = old_val, mem = old_val & 10 */
    
    /* AMOOR.W - 原子或 */
    0x0061222F,  /* amoor.w  x4, x6, (x1)  ; x4 = old_val, mem = old_val | 20 */
    
    /* AMOXOR.W - 原子异或 */
    0x006122AF,  /* amoxor.w x5, x6, (x1)  ; x5 = old_val, mem = old_val ^ 20 */
    
    /* AMOSWAP.W - 原子交换 */
    0x0061232F,  /* amoswap.w x6, x6, (x1) ; x6 = old_val, mem = 20 */
    
    /* SC.W 测试 - 条件存储 (应该失败，因为保留已失效) */
    0x005120AF,  /* sc.w  x1, x5, (x1)     ; x1 = 1 (失败) */
    
    /* ECALL 结束 */
    0x00000073,  /* ecall */
};

int main() {
    printf("=== A 扩展 (原子操作) 指令测试 ===\n\n");
    printf("测试程序：%d 条指令\n\n", (int)(sizeof(test_program) / 4));
    
    /* 输出二进制文件 */
    FILE *f = fopen("test_a.bin", "wb");
    if (!f) {
        perror("无法创建文件");
        return 1;
    }
    fwrite(test_program, sizeof(uint32_t), sizeof(test_program) / 4, f);
    fclose(f);
    
    printf("已生成：test_a.bin\n\n");
    printf("测试说明:\n");
    printf("  1. 初始化内存地址为 100\n");
    printf("  2. LR.W 加载保留字\n");
    printf("  3. AMOADD.W: 原子加法 (mem += 20)\n");
    printf("  4. AMOAND.W: 原子与 (mem &= 10)\n");
    printf("  5. AMOOR.W:  原子或 (mem |= 20)\n");
    printf("  6. AMOXOR.W: 原子异或 (mem ^= 20)\n");
    printf("  7. AMOSWAP.W:原子交换 (mem = 20)\n");
    printf("  8. SC.W: 条件存储 (预期失败，返回 1)\n");
    printf("\n运行：./rv_sim test_a.bin\n");
    
    return 0;
}
