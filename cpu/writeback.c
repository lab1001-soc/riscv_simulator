/*
 * writeback.c - 写回模块
 * 负责将执行结果写回寄存器
 * 
 * 注意：在当前设计中，大部分指令的执行结果直接在 execute.c 中写入寄存器
 * 本模块提供统一的写回接口和特殊写回操作
 */

#include "cpu.h"
#include <stdio.h>

/**
 * cpu_writeback - 通用寄存器写回
 * @cpu: CPU 状态结构体指针
 * @rd: 目标寄存器编号 (0-31)
 * @value: 要写入的值
 * 
 * 说明：x0 寄存器硬连线为 0，写入 x0 的值会被忽略
 */
void cpu_writeback(CPU *cpu, uint32_t rd, uint32_t value) {
    if (rd == 0) {
        /* x0 硬连线为 0，任何写入都被忽略 */
        return;
    }
    
    if (rd >= NUM_REGS) {
        fprintf(stderr, "[WRITEBACK] 寄存器越界：%d\n", rd);
        return;
    }
    
    cpu->regs[rd] = value;
    
#ifdef DEBUG_WRITEBACK
    printf("[WRITEBACK] x%d <- 0x%08x (%d)\n", rd, value, (int32_t)value);
#endif
}

/**
 * cpu_writeback_fp - 浮点寄存器写回
 * @cpu: CPU 状态结构体指针
 * @rd: 目标浮点寄存器编号 (0-31)
 * @value: 要写入的值 (IEEE754 格式)
 * 
 * 说明：f0 可以写入非零值
 */
void cpu_writeback_fp(CPU *cpu, uint32_t rd, uint32_t value) {
    if (rd >= NUM_FREGS) {
        fprintf(stderr, "[WRITEBACK] 浮点寄存器越界：%d\n", rd);
        return;
    }
    
    cpu->fregs[rd].u = value;
    
#ifdef DEBUG_WRITEBACK
    printf("[WRITEBACK] f%d <- 0x%08x (%.6f)\n", rd, value, cpu->fregs[rd].f);
#endif
}

/**
 * cpu_writeback_fp_float - 浮点寄存器写回 (float 值)
 * @cpu: CPU 状态结构体指针
 * @rd: 目标浮点寄存器编号 (0-31)
 * @value: 要写入的浮点数值
 */
void cpu_writeback_fp_float(CPU *cpu, uint32_t rd, float value) {
    if (rd >= NUM_FREGS) {
        fprintf(stderr, "[WRITEBACK] 浮点寄存器越界：%d\n", rd);
        return;
    }
    
    cpu->fregs[rd].f = value;
    
#ifdef DEBUG_WRITEBACK
    printf("[WRITEBACK] f%d <- %.6f (0x%08x)\n", rd, value, cpu->fregs[rd].u);
#endif
}

/**
 * cpu_writeback_pc - 程序计数器写回
 * @cpu: CPU 状态结构体指针
 * @pc: 新的 PC 值
 * 
 * 说明：PC 必须 4 字节对齐
 */
void cpu_writeback_pc(CPU *cpu, uint32_t pc) {
    if (pc & 0x3) {
        fprintf(stderr, "[WRITEBACK] PC 未对齐：0x%08x\n", pc);
        pc &= ~0x3;
    }
    
    /* PC 应在指令内存范围内 */
    if (pc < IMEM_BASE_ADDR || pc >= IMEM_BASE_ADDR + IMEM_SIZE) {
        fprintf(stderr, "[WRITEBACK] PC 越界：0x%08x\n", pc);
    }
    
    cpu->pc = pc;
    
#ifdef DEBUG_WRITEBACK
    printf("[WRITEBACK] PC <- 0x%08x\n", pc);
#endif
}

/**
 * cpu_memory_write - 内存写回
 * @cpu: CPU 状态结构体指针
 * @addr: 内存地址
 * @value: 要写入的值
 * @size: 写入大小 (1=字节，2=半字，4=字)
 * 
 * 返回：0 表示成功，-1 表示地址越界
 */
int cpu_memory_write(CPU *cpu, uint32_t addr, uint32_t value, int size) {
    /* 数据内存 (DMEM) */
    if (addr >= DMEM_BASE_ADDR && addr < DMEM_BASE_ADDR + DMEM_SIZE) {
        uint32_t dmem_addr = addr - DMEM_BASE_ADDR;
        uint32_t offset = (dmem_addr % 4) * 8;

        switch (size) {
            case 1:  /* 字节 */
                cpu->dmem[dmem_addr / 4] &= ~(0xFF << offset);
                cpu->dmem[dmem_addr / 4] |= (value & 0xFF) << offset;
                break;
            case 2:  /* 半字 */
                cpu->dmem[dmem_addr / 4] &= ~(0xFFFF << offset);
                cpu->dmem[dmem_addr / 4] |= (value & 0xFFFF) << offset;
                break;
            case 4:  /* 字 */
                cpu->dmem[dmem_addr / 4] = value;
                break;
            default:
                fprintf(stderr, "[WRITEBACK] 无效的写入大小：%d\n", size);
                return -1;
        }

#ifdef DEBUG_WRITEBACK
        printf("[WRITEBACK] DMEM[0x%08x] <- 0x%08x (size=%d)\n", addr, value, size);
#endif
        return 0;
    }

    fprintf(stderr, "[WRITEBACK] 内存地址越界：0x%08x\n", addr);
    return -1;
}

/**
 * cpu_memory_read - 内存读取 (辅助函数)
 * @cpu: CPU 状态结构体指针
 * @addr: 内存地址
 * @size: 读取大小 (1=字节，2=半字，4=字)
 * 
 * 返回：读取的值，地址越界时返回 0
 */
uint32_t cpu_memory_read(CPU *cpu, uint32_t addr, int size) {
    /* 数据内存 (DMEM) */
    if (addr >= DMEM_BASE_ADDR && addr < DMEM_BASE_ADDR + DMEM_SIZE) {
        uint32_t dmem_addr = addr - DMEM_BASE_ADDR;
        uint32_t value = cpu->dmem[dmem_addr / 4];
        uint32_t offset = (dmem_addr % 4) * 8;

        switch (size) {
            case 1:  /* 字节 */
                value = sign_extend((value >> offset) & 0xFF, 8);
                break;
            case 2:  /* 半字 */
                value = sign_extend((value >> offset) & 0xFFFF, 16);
                break;
            case 4:  /* 字 */
                /* 不需要处理 */
                break;
        }

#ifdef DEBUG_WRITEBACK
        printf("[WRITEBACK] DMEM[0x%08x] -> 0x%08x (size=%d)\n", addr, value, size);
#endif
        return value;
    }

    fprintf(stderr, "[WRITEBACK] 内存地址越界：0x%08x\n", addr);
    return 0;
}
