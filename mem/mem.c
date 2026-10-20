/*
 * mem.c - 内存管理模块实现
 * 统一管理 IMEM/DMEM/UART 的读写访问
 * 取指令可以从 IMEM 或 DMEM 中获取
 */

#include "mem.h"
#include "cpu.h"
#include "mmu.h"
#include <stdio.h>
#include <string.h>

/* ==================== 内存区域检测 ==================== */

/**
 * mem_is_imem - 检测地址是否在 IMEM 区域
 */
int mem_is_imem(uint32_t addr) {
    return addr >= IMEM_BASE_ADDR && addr < IMEM_BASE_ADDR + IMEM_SIZE;
}

/**
 * mem_is_dmem - 检测地址是否在 DMEM 区域
 */
int mem_is_dmem(uint32_t addr) {
    return addr >= DMEM_BASE_ADDR && addr < DMEM_BASE_ADDR + DMEM_SIZE;
}

int mem_is_lmem(uint32_t addr) {
    return addr >= LMEM_BASE_ADDR && addr < LMEM_BASE_ADDR + LMEM_SIZE;
}

/**
 * mem_is_valid - 检测地址是否在有效内存区域 (IMEM / DMEM / LMEM)
 */
int mem_is_valid(uint32_t addr) {
    return mem_is_imem(addr) || mem_is_dmem(addr) || mem_is_lmem(addr);
}

/* ==================== 物理内存读写 (绕过 MMU) ==================== */

/**
 * mem_read_physical - 绕过 MMU，直接物理地址读
 * 页表 walker 内部使用，不经过 va→pa 翻译
 */
uint32_t mem_read_physical(CPU *cpu, uint32_t pa, int size) {

    uint32_t ret = 0;

    /* 遍历外设 */
    for (int i = 0; i < cpu->num_peripherals; i++) {
        Peripheral *p = cpu->peripherals[i];
        if (p->match(p, pa)) {
            ret = p->read(p, pa, size, cpu);
            goto END;
        }
    }

    /* IMEM 区域 */
    if (mem_is_imem(pa)) {
        uint32_t offset = pa - IMEM_BASE_ADDR;
        uint32_t shift = (offset % 4) * 8;
        uint32_t word = cpu->imem[offset / 4];
        switch (size) {
            case 1: ret =  (word >> shift) & 0xFF;     break;
            case 2: ret =  (word >> shift) & 0xFFFF;   break;
            case 4: ret =  word;                       break;
        }
        goto END;
    }

    /* DMEM 区域 */
    if (mem_is_dmem(pa)) {
        uint32_t offset = pa - DMEM_BASE_ADDR;
        uint32_t shift = (offset % 4) * 8;
        uint32_t word = cpu->dmem[offset / 4];
        switch (size) {
            case 1: ret = (word >> shift) & 0xFF;       break;
            case 2: ret = (word >> shift) & 0xFFFF;     break;
            case 4: ret = word;                         break;
        }
    }

    /* LMEM 区域 */
    if (mem_is_lmem(pa)) {
        uint32_t offset = pa - LMEM_BASE_ADDR;
        uint32_t shift = (offset % 4) * 8;
        uint32_t word = cpu->lmem[offset / 4];
        switch (size) {
            case 1: ret = (word >> shift) & 0xFF;       break;
            case 2: ret = (word >> shift) & 0xFFFF;     break;
            case 4: ret = word;                         break;
        }
    }
END:
#ifdef MEM_TRACE
     printf("    [mr:0x%08x] 0x%08x-%d\n", pa, ret, size);
#endif
    cpu_halt_det(cpu, 1, pa);
    return ret;
}

void mem_write_physical(CPU *cpu, uint32_t pa, uint32_t value, int size) {
#ifdef MEM_TRACE
     printf("    [mw:0x%08x] 0x%08x-%d\n", pa, value, size);
#endif
    cpu_halt_det(cpu, 2, pa);

    /* 遍历外设 */
    for (int i = 0; i < cpu->num_peripherals; i++) {
        Peripheral *p = cpu->peripherals[i];
        if (p->match(p, pa)) {
            p->write(p, pa, value, size, cpu);
            return;
        }
    }

    /* IMEM 区域 */
    if (mem_is_imem(pa)) {
        uint32_t offset = pa - IMEM_BASE_ADDR;
        uint32_t shift = (offset % 4) * 8;
        switch (size) {
            case 1:
                cpu->imem[offset / 4] &= ~(0xFF << shift);
                cpu->imem[offset / 4] |= (value & 0xFF) << shift;
                break;
            case 2:
                cpu->imem[offset / 4] &= ~(0xFFFF << shift);
                cpu->imem[offset / 4] |= (value & 0xFFFF) << shift;
                break;
            case 4:
                cpu->imem[offset / 4] = value;
                break;
        }
        printf("WARNING:pc=0x%08x    [imem:0x%08x] 0x%08x-%d\n", cpu->pc, pa, value, size);
        return;
    }

    /* DMEM 区域 */
    if (mem_is_dmem(pa)) {
        uint32_t offset = pa - DMEM_BASE_ADDR;
        uint32_t shift = (offset % 4) * 8;
        switch (size) {
            case 1:
                cpu->dmem[offset / 4] &= ~(0xFF << shift);
                cpu->dmem[offset / 4] |= (value & 0xFF) << shift;
                break;
            case 2:
                cpu->dmem[offset / 4] &= ~(0xFFFF << shift);
                cpu->dmem[offset / 4] |= (value & 0xFFFF) << shift;
                break;
            case 4:
                cpu->dmem[offset / 4] = value;
                break;
        }
    }

    /* LMEM 区域 */
    if (mem_is_lmem(pa)) {
        uint32_t offset = pa - LMEM_BASE_ADDR;
        uint32_t shift = (offset % 4) * 8;
        switch (size) {
            case 1:
                cpu->lmem[offset / 4] &= ~(0xFF << shift);
                cpu->lmem[offset / 4] |= (value & 0xFF) << shift;
                break;
            case 2:
                cpu->lmem[offset / 4] &= ~(0xFFFF << shift);
                cpu->lmem[offset / 4] |= (value & 0xFFFF) << shift;
                break;
            case 4:
                cpu->lmem[offset / 4] = value;
                break;
        }
    }
}

/* ==================== 统一内存读写 (带 MMU 翻译) ==================== */

/**
 * mem_read - 统一内存读取 (VA→PA 翻译)
 */
uint32_t mem_read(CPU *cpu, uint32_t addr, int size) {
    uint32_t pa;
    if (mmu_translate(cpu, addr, &pa, MMU_LOAD) != 0) {
        /* 页错误 → 触发 trap，trap 后 PC 会被改写 */
        cpu_trap(cpu, CAUSE_LOAD_PAGE_FAULT, addr);
        return 0;
    }
    return mem_read_physical(cpu, pa, size);
}

/* ==================== 统一内存写入 (带 MMU 翻译) ==================== */

/**
 * mem_write - 统一内存写入 (VA→PA 翻译)
 */
void mem_write(CPU *cpu, uint32_t addr, uint32_t value, int size) {
    uint32_t pa;
    if (mmu_translate(cpu, addr, &pa, MMU_STORE) != 0) {
        cpu_trap(cpu, CAUSE_STORE_PAGE_FAULT, addr);
        return;
    }
    mem_write_physical(cpu, pa, value, size);
}

/* ==================== 取指令 (带 MMU 翻译) ==================== */

/**
 * mem_fetch_instr - 取指令 (VA→PA 翻译后从 IMEM/DMEM 获取)
 */
uint32_t mem_fetch_instr(CPU *cpu, uint32_t pc, int *instr_len) {
    /* VA→PA 翻译 */
    uint32_t pa;
    if (mmu_translate(cpu, pc, &pa, MMU_FETCH) != 0) {
        cpu_trap(cpu, CAUSE_INSTRUCTION_PAGE_FAULT, pc);
        *instr_len = 0;
        return 0;
    }

    /* PC 必须在 IMEM 或 DMEM 区域 */
    if (!mem_is_valid(pa)) {
        fprintf(stderr, "[FETCH] PC 越界：0x%08x (→PA: 0x%08x)\n", pc, pa);
        *instr_len = 0;
        return 0;
    }

    /* PC 必须 2 字节对齐 */
    if (pa & 0x1) {
        fprintf(stderr, "[FETCH] PC 未对齐：0x%08x\n", pa);
        *instr_len = 0;
        return 0;
    }

    uint32_t offset, word, next_word, instr;
    uint16_t instr_low;

    /* 从 IMEM / DMEM / LMEM 取指令 */
    if (mem_is_imem(pa)) {
        offset = pa - IMEM_BASE_ADDR;
        word = cpu->imem[offset / 4];
        next_word = cpu->imem[offset / 4 + 1];
    } else if (mem_is_dmem(pa)) {
        offset = pa - DMEM_BASE_ADDR;
        word = cpu->dmem[offset / 4];
        next_word = cpu->dmem[offset / 4 + 1];
    } else {
        offset = pa - LMEM_BASE_ADDR;
        word = cpu->lmem[offset / 4];
        next_word = cpu->lmem[offset / 4 + 1];
    }

    /* 根据地址确定取低半字还是高半字 */
    if ((offset & 0x2) == 0)
        instr_low = word & 0xFFFF;
    else
        instr_low = (word >> 16) & 0xFFFF;

    /* 压缩指令检测 */
    if ((instr_low & 0x3) != 0x3) {
        *instr_len = 2;
        instr = instr_low;
    } else {
        *instr_len = 4;
        if ((offset & 0x2) == 0)
            instr = word;
        else
            instr = instr_low | ((next_word & 0xFFFF) << 16);
    }

    return instr;
}

/* ==================== 程序加载 ==================== */

/**
 * mem_load_program - 加载程序到内存
 * @cpu: CPU 状态结构体指针
 * @program: 程序数据指针
 * @size: 程序大小 (字节)
 * @base_addr: 加载基地址
 *
 * 返回：0 表示成功，-1 表示失败
 */
int mem_load_program(CPU *cpu, const uint8_t *program, size_t size, uint32_t base_addr) {
    /* 检查目标区域 */
    if (mem_is_imem(base_addr)) {
        if (size > IMEM_SIZE) {
            fprintf(stderr, "[MEM] 程序太大：%lu 字节 (IMEM 最大 %d)\n",
                    (unsigned long)size, IMEM_SIZE);
            return -1;
        }
        uint32_t offset = base_addr - IMEM_BASE_ADDR;
        memcpy((uint8_t*)cpu->imem + offset, program, size);
        return 0;
    }

    if (mem_is_dmem(base_addr)) {
        if (size > DMEM_SIZE) {
            fprintf(stderr, "[MEM] 程序太大：%lu 字节 (DMEM 最大 %d)\n",
                    (unsigned long)size, DMEM_SIZE);
            return -1;
        }
        uint32_t offset = base_addr - DMEM_BASE_ADDR;
        memcpy((uint8_t*)cpu->dmem + offset, program, size);
        return 0;
    }

    if (mem_is_lmem(base_addr)) {
        if (size > (size_t)cpu->lmem_size) {
            fprintf(stderr, "[MEM] 程序太大：%lu 字节 (LMEM 最大 %d)\n",
                    (unsigned long)size, (int)cpu->lmem_size);
            return -1;
        }
        uint32_t offset = base_addr - LMEM_BASE_ADDR;
        memcpy((uint8_t*)cpu->lmem + offset, program, size);
        return 0;
    }

    fprintf(stderr, "[MEM] 加载地址无效：0x%08x\n", base_addr);
    return -1;
}