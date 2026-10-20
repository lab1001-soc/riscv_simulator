/*
 * fetch.c - 取指模块
 * 负责从内存中获取指令，支持压缩指令(C扩展)
 */

#include "cpu.h"
#include "mem.h"
#include <stdio.h>

/**
 * is_compressed - 检测是否为压缩指令
 * @instr_low: 指令低 16 位
 *
 * 返回：1 表示压缩指令，0 表示标准 32 位指令
 *
 * 压缩指令最低 2 位 != 11
 */
int is_compressed(uint16_t instr_low) {
    return (instr_low & 0x3) != 0x3;
}

/**
 * cpu_fetch - 从内存中取指
 * @cpu: CPU 状态结构体指针
 * @instr_len: 输出参数，返回指令长度（2 或 4）
 *
 * 返回：32 位指令（压缩指令会扩展为等效的 32 位格式），如果 PC 超出内存范围返回 0
 */
uint32_t cpu_fetch(CPU *cpu, int *instr_len) {
    /* 使用统一内存模块取指令 */
    uint32_t instr = mem_fetch_instr(cpu, cpu->pc, instr_len);

#ifdef DEBUG_FETCH
    printf("[FETCH] PC=0x%08x, INSTR=0x%08x (len=%d)\n", cpu->pc, instr, *instr_len);
#endif

    return instr;
}