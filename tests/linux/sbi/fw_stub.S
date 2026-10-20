/* fw_stub.S — SBI v0.2 firmware
 * Build: riscv32-unknown-elf-gcc -march=rv32imafc -mabi=ilp32f
 *          -nostdlib -T fw_stub.ld -o fw_stub.elf fw_stub.S
 *        riscv32-unknown-elf-objcopy -O binary fw_stub.elf fw_stub.bin
 */

    .section .text
    .globl _start
_start:
    /* M-mode stack at 0x87FFF000 (top of 128MB LMEM - 4KB) */
    li   t0, 0x87FFF000
    csrw 0x340, t0       /* mscratch = M-mode stack top */

    /* Clear pending interrupts */
    csrw 0x344, zero

    /* mideleg: delegate SSIP(1), STIP(5), SEIP(9) to S-mode */
    la   t0, cfg_mideleg
    lw   t0, 0(t0)
    csrw 0x303, t0

    /* medeleg: delegate U-ecall(8), page faults(12,13,15) */
    la   t0, cfg_medeleg
    lw   t0, 0(t0)
    csrw 0x302, t0

    /* mtvec -> SBI handler */
    la   t0, sbi_handler
    csrw 0x305, t0

    /* mstatus: FS(0x6000)+MPP_S(0x800)+MPIE(0x80)=0x6880 */
    la   t0, cfg_mstatus
    lw   t0, 0(t0)
    csrw 0x300, t0

    /* mepc = kernel entry */
    la   t0, cfg_kernel
    lw   t0, 0(t0)
    csrw 0x341, t0

    /* a0=hartid=0, a1=FDT */
    li   a0, 0
    la   t0, cfg_fdt
    lw   a1, 0(t0)

    /* Print "FWOK\r\n" */
    la   t2, msg_fw
    la   t3, cfg_uart
    lw   t3, 0(t3)
1:  lb   t1, 0(t2)
    beqz t1, 2f
    sb   t1, 0(t3)
    addi t2, t2, 1
    j    1b
2:  mret

    .section .rodata
msg_fw:     .asciz "FWOK\r\n"

    .section .data
    .align 4
cfg_mideleg: .word 0x00000222     /* SSIP|STIP|SEIP */
cfg_medeleg: .word 0x0000B100     /* U-ecall + page faults */
cfg_mstatus: .word 0x00006880     /* FS+MPP_S+MPIE */
cfg_kernel:  .word 0x80000000     /* FW_JUMP_ADDR */
cfg_fdt:     .word 0x1001000      /* FDT_ADDR */
cfg_uart:    .word 0x00008200     /* UART_TX */
cfg_mtmc_lo: .word 0x00008308     /* MTIMECMP_LO */
cfg_mtmc_hi: .word 0x0000830C     /* MTIMECMP_HI */

/* ================================================================
 * SBI Trap Handler — processes ECALL from S-mode
 * v0.2:  a7=extension, a6=function → return a0=error a1=value
 * Legacy: a7=fid → return a0=0 on success
 *
 * Stack frame: sp+12=a0, sp+8=a1, sp+4=a7, sp+0=ra
 * ================================================================ */
    .align 2
sbi_handler:
    /* swap sp and mscratch to get M-mode stack */
    csrrw sp, 0x340, sp
    addi sp, sp, -16
    sw   ra, 0(sp)
    sw   a7, 4(sp)
    sw   a1, 8(sp)
    sw   a0, 12(sp)

    /* Return immediately if mcause != 9 (S-ecall) */
    csrr t0, 0x342
    addi t1, zero, 9
    bne  t0, t1, sbi_exit

    /* ── v0.2: a7 >= 0x10 ── */
    addi t1, zero, 0x10
    bltu a7, t1, legacy

    /* Base extension 0x10 */
    addi t0, zero, 0x10
    beq  a7, t0, base_ext

    /* Timer extension 0x54494D45 */
    la   t0, ext_time
    lw   t0, 0(t0)
    beq  a7, t0, timer_ext

    /* Unknown → NOT_SUPPORTED（同样需要存栈槽） */
    li   a0, -2
    sw   a0, 12(sp)
    j    sbi_exit

/* ─── Legacy SBI (a7 < 0x10) ─── */
legacy:
    bnez a7, 1f
    /* 0: set_timer(a0=lo, a1=hi) */
    la   t3, cfg_mtmc_lo; lw t3,0(t3); sw a0,0(t3)
    la   t3, cfg_mtmc_hi; lw t3,0(t3); sw a1,0(t3)
    j    sbi_ok

1:  addi t0, zero, 1; bne a7,t0,2f
    /* 1: putchar(a0) */
    la   t3, cfg_uart; lw t3,0(t3); sb a0,0(t3)
    j    sbi_ok

2:  addi t0, zero, 2; bne a7,t0,3f
    /* 2: getchar — 查 UART RX，无数据返回 -1
     * UART_SR = 0x8214 (bit0=rx_valid), UART_RX = 0x8210 (读后自动出队)
     * 之前固定返回 -1 → 内核 tty 收不到键盘输入 → shell 无法交互。
     * 注意返回值必须存栈槽（Bug 14），否则 sbi_exit 恢复调用者 a0。 */
    li   t3, 0x8214           /* UART SR 地址 */
    lw   t4, 0(t3)            /* 读状态 */
    andi t4, t4, 1            /* bit0 = rx_valid */
    beqz t4, 22f              /* 无数据 → 返回 -1 */
    li   t3, 0x8210           /* UART RX 地址 */
    lw   a0, 0(t3)            /* 读字符（自动出队） */
    andi a0, a0, 0xFF         /* 只取低 8 位 */
    j    21f
22: li   a0, -1               /* 无数据 */
21: sw   a0, 12(sp)           /* 存返回值到栈槽 */
    j    sbi_exit

3:  /* SBI v0.1 legacy: 5=remote_fence_i, 6=remote_sfence_vma,
     * 7=remote_sfence_vma_asid — 单核下广播目标就是本 hart，
     * 必须真正执行 sfence.vma。Bug 12 根因：内核 set_memory_* 修改页表
     * 属性后靠此调用刷 TLB，之前假返回成功 → 模拟器 TLB 残留旧 RX 条目，
     * free_initmem 毒化 memset 撞上 W=0 旧条目 → store page fault。
     * 只有 8=shutdown 才进入 wfi 死循环。 */
    addi t0, zero, 8; bne a7,t0,4f
99: wfi; j .       /* shutdown (fid 8) */
4:  /* fid 3/4 (clear_ipi/send_ipi) 无事可做；
     * fid 5/6/7 (fence_i / sfence_vma[/asid]) 单核=本 hart，真刷 TLB */
    slti t0, a7, 5
    bnez t0, 5f
    sfence.vma
5:  j    sbi_ok

/* ─── SBI Base Extension (0x10) ─── */
base_ext:
    addi t0, zero, 0; beq a6,t0,b0   /* get_spec_version */
    addi t0, zero, 1; beq a6,t0,b1   /* get_impl_id */
    addi t0, zero, 2; beq a6,t0,b2   /* get_impl_version */
    addi t0, zero, 3; beq a6,t0,b3   /* probe_extension */
    addi t0, zero, 4; beq a6,t0,b4   /* get_mvendorid */
    li   a0, -2
    sw   a0, 12(sp)
    j    sbi_exit

b0: /* SBI v0.2: major=0, minor=2 */
    li   a1, 2; j base_val
b1: li   a1, 0; j base_val       /* BBL implementation */
b2: li   a1, 0; j base_val       /* impl version 0 */
b3: /* probe(a0=ext_id): BASE(0x10)=1, TIME=1, others=0 */
    addi t0, zero, 0x10
    beq  a0, t0, base_yes
    la   t0, ext_time
    lw   t1, 0(t0)
    beq  a0, t1, base_yes
    li   a1, 0; j base_val
base_yes: li a1, 1; j base_val
b4: csrr a1, 0xF11; j base_val  /* mvendorid */
base_val: sw a1, 8(sp)
sbi_ok:    li a0, 0
           sw a0, 12(sp)         /* store return value to stack */
    j    sbi_exit

/* ─── SBI Timer Extension ─── */
timer_ext:
    bnez a6, 9f
    la   t3, cfg_mtmc_lo; lw t3,0(t3); sw a0,0(t3)
    la   t3, cfg_mtmc_hi; lw t3,0(t3); sw a1,0(t3)
    j    sbi_ok
9:  li   a0, -2
    sw   a0, 12(sp)

/* ─── Return path ─── */
sbi_exit:
    csrr t0, 0x341; addi t0, t0, 4; csrw 0x341, t0
    lw   ra, 0(sp)
    lw   a7, 4(sp)
    lw   a1, 8(sp)
    lw   a0, 12(sp)
    addi sp, sp, 16
    /* swap back: restore kernel sp, save M-mode stack top in mscratch */
    csrrw sp, 0x340, sp
    mret

    .section .data
    .align 4
ext_time: .word 0x54494D45

    /* M-mode stack: see li sp, 0x83FFF000 at _start */
