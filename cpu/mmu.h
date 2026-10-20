/*
 * mmu.h - Sv32 MMU 接口
 * 两级页表 walker + 简化 TLB
 *
 * Sv32 虚拟地址拆分:
 *   VA[31:22] = VPN[1] → 第一级页表索引
 *   VA[21:12] = VPN[0] → 第二级页表索引
 *   VA[11:0]  = offset  → 页内偏移
 *
 * PTE 格式 (32-bit):
 *   bits[31:10] = PPN
 *   bits[9:8]   = RSW (reserved for supervisor)
 *   bit 7 = D (dirty)
 *   bit 6 = A (accessed)
 *   bit 5 = G (global)
 *   bit 4 = U (user)
 *   bit 3 = X (executable)
 *   bit 2 = W (writable)
 *   bit 1 = R (readable)
 *   bit 0 = V (valid)
 */

#ifndef MMU_H
#define MMU_H

#include <stdint.h>
#include "config.h"

/* ===== 页大小 ===== */
#define PAGE_SIZE           4096
#define PAGE_SHIFT          12
#define PAGE_MASK           0xFFFFF000u
#define PAGE_OFFSET_MASK    0x00000FFFu

/* ===== SATP 位 ===== */
#define SATP_MODE_BARE      0x00000000u
#define SATP_MODE_SV32      0x80000000u
#define SATP_MODE_MASK      0x80000000u
#define SATP_ASID_MASK      0x7FC00000u
#define SATP_ASID_SHIFT     22
#define SATP_PPN_MASK       0x003FFFFFu

/* ===== Sv32 虚拟地址拆分 ===== */
#define VPN1_SHIFT          22
#define VPN0_SHIFT          12
#define VPN1(va)            (((va) >> VPN1_SHIFT) & 0x3FF)
#define VPN0(va)            (((va) >> VPN0_SHIFT) & 0x3FF)
#define OFFSET(va)          ((va) & 0xFFF)

/* ===== PTE 位 ===== */
#define PTE_V               (1u << 0)
#define PTE_R               (1u << 1)
#define PTE_W               (1u << 2)
#define PTE_X               (1u << 3)
#define PTE_U               (1u << 4)
#define PTE_G               (1u << 5)
#define PTE_A               (1u << 6)
#define PTE_D               (1u << 7)
#define PTE_PPN_MASK        0xFFFFFC00u
#define PTE_PPN_SHIFT       10

#define PTE_RWX_MASK        (PTE_R | PTE_W | PTE_X)

/* ===== TLB 层级 ===== *
 * ITLB (32-entry): 指令 fetch 专用, direct-mapped
 * DTLB (32-entry): load/store 专用, direct-mapped
 * STLB (128-entry): L2 统一缓存, direct-mapped
 *
 * 查找流程:
 *   IF:   ITLB → miss → STLB → miss → HPTW(页表walk)
 *   LD/ST: DTLB → miss → STLB → miss → HPTW(页表walk)
 *   HPTW 结果填充 STLB + 请求的 I/D TLB
 *   sfence.vma: 全部清空
 */
#define ITLB_SIZE           32
#define DTLB_SIZE           32
#define STLB_SIZE           128

typedef struct {
    uint32_t vpn;       /* 虚拟页号 (VA[31:12]) */
    uint32_t ppn;       /* 物理页号 (PA[31:12]) */
    uint32_t pte;       /* 原始 PTE (含权限位) */
    int      valid;
} TLB_Entry;

/* ===== 访问类型 ===== */
#define MMU_FETCH  0
#define MMU_LOAD   1
#define MMU_STORE  2

/* ===== 函数声明 ===== */
struct CPU;

int  mmu_translate (struct CPU *cpu, uint32_t va, uint32_t *pa, int access_type);
int  mmu_enabled   (struct CPU *cpu);
void mmu_flush_tlb (struct CPU *cpu);

/* 物理内存直读直写 (绕过 MMU，供页表 walker 使用) */
uint32_t mem_read_physical (struct CPU *cpu, uint32_t pa, int size);
void     mem_write_physical(struct CPU *cpu, uint32_t pa, uint32_t value, int size);

#endif /* MMU_H */
