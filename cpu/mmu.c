/*
 * mmu.c - Sv32 MMU 实现
 *
 * TLB 三级架构:
 *   ITLB (32-entry)  ←── 指令 fetch 专用 L1
 *   DTLB (32-entry)  ←── load/store 专用 L1
 *   STLB (128-entry) ←── L2 统一缓存, I/D miss 后查询
 *   HPTW            ←── 硬件页表 walker, STLB miss 后触发
 *
 * 查找流程:
 *   取指 → ITLB 命中 → 返回
 *   取指 → ITLB miss → STLB 命中 → 回填 ITLB + 返回
 *   取指 → STLB miss → HPTW 走页表 → 回填 STLB+ITLB
 *
 *   ld/st → DTLB 命中 → 返回
 *   ld/st → DTLB miss → STLB 命中 → 回填 DTLB + 返回
 *   ld/st → STLB miss → HPTW 走页表 → 回填 STLB+DTLB
 */

#include "mmu.h"
#include "cpu.h"
#include "mem.h"
#include <stdio.h>
#include <string.h>

/* ===== L1 ITLB ===== */

static TLB_Entry* itlb_lookup(CPU *cpu, uint32_t vpn) {
    int idx = vpn % ITLB_SIZE;
    TLB_Entry *e = &cpu->itlb[idx];
    return (e->valid && e->vpn == vpn) ? e : NULL;
}

static void itlb_insert(CPU *cpu, uint32_t vpn, uint32_t ppn, uint32_t pte) {
    int idx = vpn % ITLB_SIZE;
    cpu->itlb[idx].vpn   = vpn;
    cpu->itlb[idx].ppn   = ppn;
    cpu->itlb[idx].pte   = pte;
    cpu->itlb[idx].valid = 1;
}

/* ===== L1 DTLB ===== */

static TLB_Entry* dtlb_lookup(CPU *cpu, uint32_t vpn) {
    int idx = vpn % DTLB_SIZE;
    TLB_Entry *e = &cpu->dtlb[idx];
    return (e->valid && e->vpn == vpn) ? e : NULL;
}

static void dtlb_insert(CPU *cpu, uint32_t vpn, uint32_t ppn, uint32_t pte) {
    int idx = vpn % DTLB_SIZE;
    cpu->dtlb[idx].vpn   = vpn;
    cpu->dtlb[idx].ppn   = ppn;
    cpu->dtlb[idx].pte   = pte;
    cpu->dtlb[idx].valid = 1;
}

/* ===== L2 STLB (Shared TLB, ITLB/DTLB miss 后查询) ===== */

static TLB_Entry* stlb_lookup(CPU *cpu, uint32_t vpn) {
    int idx = vpn % STLB_SIZE;
    TLB_Entry *e = &cpu->stlb[idx];
    return (e->valid && e->vpn == vpn) ? e : NULL;
}

static void stlb_insert(CPU *cpu, uint32_t vpn, uint32_t ppn, uint32_t pte) {
    int idx = vpn % STLB_SIZE;
    cpu->stlb[idx].vpn   = vpn;
    cpu->stlb[idx].ppn   = ppn;
    cpu->stlb[idx].pte   = pte;
    cpu->stlb[idx].valid = 1;
#ifdef TLB_TRACE_C0819
    if (vpn == 0xc0819)
        printf("[TLB] stlb insert vpn=c0819 pte=0x%08x pc=0x%08x\n", pte, cpu->pc);
#endif
}

/* ===== 全 TLB 刷新 (sfence.vma) ===== */

void mmu_flush_tlb(CPU *cpu) {
#ifdef TLB_TRACE_C0819
    printf("[TLB] flush pc=0x%08x (stlb c0819: ", cpu->pc);
    if (cpu->stlb[0xc0819 % STLB_SIZE].valid &&
        cpu->stlb[0xc0819 % STLB_SIZE].vpn == 0xc0819)
        printf("pte=0x%08x", cpu->stlb[0xc0819 % STLB_SIZE].pte);
    else
        printf("-");
    printf(")\n");
#endif
    memset(cpu->itlb, 0, sizeof(cpu->itlb));
    memset(cpu->dtlb, 0, sizeof(cpu->dtlb));
    memset(cpu->stlb, 0, sizeof(cpu->stlb));
}

/* ===== HPTW: 硬件页表 walker ===== *
 * 走 Sv32 两级页表，返回 ppn+pte
 * 成功返回 0，失败返回 -1
 */
static int hptw_walk(CPU *cpu, uint32_t va, uint32_t *out_ppn,
                     uint32_t *out_pte, int access_type) {
    uint32_t satp     = cpu->satp;
    uint32_t root_ppn = satp & SATP_PPN_MASK;

    /* L1: 第一级 PTE */
    uint32_t l1_addr = (root_ppn << PAGE_SHIFT) | (VPN1(va) * 4);
    uint32_t l1_pte  = mem_read_physical(cpu, l1_addr, 4);

    /* RISC-V spec: non-leaf PTE must have R=W=X=0, V=1 */
    if (!(l1_pte & PTE_V)) {
        return -1;
    }

    /* Sv32 mega-page: if L1 PTE has any RWX bit set, it's a 4MB leaf */
    if ((l1_pte & PTE_RWX_MASK) != 0) {
        /* Mega-page: 4MB mapping, VA[21:0] is the page offset.
         * Merge VPN0 into PPN so the 4KB-based pa calc works:
         *   pa = (ppn << 12) | (va & 0xFFF)
         *   where ppn = base_ppn + VPN0(va) */
        uint32_t base_ppn = (l1_pte & PTE_PPN_MASK) >> PTE_PPN_SHIFT;
        uint32_t vpn0 = VPN0(va);
        *out_ppn = base_ppn + vpn0;
        *out_pte = l1_pte;
        return 0;
    }

    /* L2: 第二级 PTE */
    uint32_t l2_ppn  = l1_pte >> PTE_PPN_SHIFT;
    uint32_t l2_addr = (l2_ppn << PAGE_SHIFT) | (VPN0(va) * 4);
    uint32_t l2_pte  = mem_read_physical(cpu, l2_addr, 4);

    if (!(l2_pte & PTE_V)) {
        return -1;
    }
    if (!(l2_pte & PTE_R) && (l2_pte & PTE_W)) {
        return -1;
    }
    if (!(l2_pte & (PTE_R | PTE_X))) {
        return -1;
    }

    /* 更新 A/D 位 */
    uint32_t new_pte = l2_pte;
    int need_update  = 0;
    if (access_type == MMU_STORE) {
        if (!(new_pte & PTE_A)) { new_pte |= PTE_A; need_update = 1; }
        if (!(new_pte & PTE_D)) { new_pte |= PTE_D; need_update = 1; }
    } else {
        if (!(new_pte & PTE_A)) { new_pte |= PTE_A; need_update = 1; }
    }
    if (need_update)
        mem_write_physical(cpu, l2_addr, new_pte, 4);

    *out_ppn = (l2_pte & PTE_PPN_MASK) >> PTE_PPN_SHIFT;
    *out_pte = l2_pte;
    return 0;
}

/* ===== TLB 条目新鲜度校验 =====
 * Bug 12 根因：内核 set_memory_* 修改页表属性（RX→RW）后，单核下 sfence
 * 的时序组合仍会给 TLB 留下未刷新的旧权限条目 → store page fault。
 * 这里采用保守模型：TLB 命中时重走页表，对比 ppn + V/R/W/X/U 权限位，
 * 不一致视为过期条目（作废后按 miss 重新走）。页表永远是权威数据源。 */
static int tlb_entry_fresh(CPU *cpu, TLB_Entry *e) {
    uint32_t ppn, pte;
    const uint32_t mask = PTE_V | PTE_R | PTE_W | PTE_X | PTE_U;
    if (hptw_walk(cpu, e->vpn << PAGE_SHIFT, &ppn, &pte, MMU_LOAD) != 0)
        return 0;   /* 页表已无此映射 → 过期 */
    return ppn == e->ppn && (pte & mask) == (e->pte & mask);
}

/* ===== PTE 权限检查 ===== */

static int pte_perm_check(CPU *cpu, uint32_t pte, int access_type,
                          uint32_t eff_priv) {
    /* U-bit */
    if (eff_priv == PRIV_U && !(pte & PTE_U))
        return 0;
    if (eff_priv == PRIV_S && (pte & PTE_U)
                          && !(cpu->mstatus & MSTATUS_SUM))
        return 0;

    /* RWX */
    if (access_type == MMU_FETCH && !(pte & PTE_X))
        return 0;
    if (access_type == MMU_LOAD) {
        if (!(pte & PTE_R) &&
            !((cpu->mstatus & MSTATUS_MXR) && (pte & PTE_X)))
            return 0;
    }
    if (access_type == MMU_STORE && !(pte & PTE_W))
        return 0;

    return 1;  /* 权限通过 */
}

/* ===== mmu_translate: 三级 TLB 查找入口 ===== */

int mmu_enabled(CPU *cpu) {
    return ((cpu->satp & SATP_MODE_MASK) == SATP_MODE_SV32);
}

int mmu_translate(CPU *cpu, uint32_t va, uint32_t *pa, int access_type) {
    /* Bare 模式: VA=PA */
    if (!mmu_enabled(cpu)) {
        *pa = va;
        return 0;
    }

    /* 模拟流水线延迟：csrw satp 后，对于物理地址段内的 VA
     * 使用恒等映射，直到首次访问内核虚拟地址 (>= 0xC0000000) */
    if (cpu->mmu_skip) {
        if (va < 0xC0000000) {
            /* 仍在物理地址范围，保持恒等映射 */
            *pa = va;
            return 0;
        } else {
            /* 首次访问虚拟地址，正式启用 MMU */
            cpu->mmu_skip = 0;
        }
    }

    /* MPRV: 使用 MPP 做权限检查 */
    uint32_t eff_priv = cpu->privilege;
    if (eff_priv == PRIV_M && (cpu->mstatus & MSTATUS_MPRV))
        eff_priv = (cpu->mstatus >> 11) & 0x3;

    /* M-mode: 绕过 MMU */
    if (eff_priv == PRIV_M) {
        *pa = va;
        return 0;
    }

    uint32_t vpn = va >> PAGE_SHIFT;
    TLB_Entry *hit = NULL;

    /* ─── 阶段 1: 查 L1 (ITLB 或 DTLB) ─── */
    if (access_type == MMU_FETCH)
        hit = itlb_lookup(cpu, vpn);
    else
        hit = dtlb_lookup(cpu, vpn);

    if (hit && !tlb_entry_fresh(cpu, hit)) {
        hit->valid = 0;   /* 过期条目作废，按 miss 继续 */
        hit = NULL;
    }

    if (hit) {
        /* L1 命中 → 权限检查 → 返回 */
        if (!pte_perm_check(cpu, hit->pte, access_type, eff_priv))
            return -1;
        *pa = (hit->ppn << PAGE_SHIFT) | (va & PAGE_OFFSET_MASK);
        return 0;
    }

    /* ─── 阶段 2: L1 miss → 查 L2 STLB ─── */
    hit = stlb_lookup(cpu, vpn);
    if (hit && !tlb_entry_fresh(cpu, hit)) {
        hit->valid = 0;   /* 过期条目作废，按 miss 继续 */
        hit = NULL;
    }

    if (hit) {
        /* STLB 命中 → 回填 L1 + 返回 */
        if (!pte_perm_check(cpu, hit->pte, access_type, eff_priv))
            return -1;
        if (access_type == MMU_FETCH)
            itlb_insert(cpu, vpn, hit->ppn, hit->pte);
        else
            dtlb_insert(cpu, vpn, hit->ppn, hit->pte);
        *pa = (hit->ppn << PAGE_SHIFT) | (va & PAGE_OFFSET_MASK);
        return 0;
    }

    /* ─── 阶段 3: STLB miss → HPTW 走页表 ─── */
    uint32_t ppn, pte;
    if (hptw_walk(cpu, va, &ppn, &pte, access_type) != 0) {
        /* HPTW 失败但 VA 像物理地址 → 恒等映射 */
        if (va >= LMEM_BASE_ADDR && va < LMEM_BASE_ADDR + LMEM_SIZE) {
            *pa = va;
            return 0;
        }
        return -1;
    }

    /* HPTW 成功 → 回填 STLB + L1 */
    stlb_insert(cpu, vpn, ppn, pte);
    if (access_type == MMU_FETCH)
        itlb_insert(cpu, vpn, ppn, pte);
    else
        dtlb_insert(cpu, vpn, ppn, pte);

    /* 权限检查 */
    if (!pte_perm_check(cpu, pte, access_type, eff_priv))
        return -1;

    *pa = (ppn << PAGE_SHIFT) | (va & PAGE_OFFSET_MASK);
    return 0;
}
