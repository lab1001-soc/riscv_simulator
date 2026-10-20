# See LICENSE for license details.
#
# test_isa.S - Merged RV32 IMACF ISA Test Suite
#
# All tests are callable C functions returning 0 (pass) or (testnum<<1)|1 (fail).
# Call test_isa() from test_isa.c to run all tests.

#include "riscv_test.h"
#include "test_macros.h"

# ======================================================================
# Macro Overrides - disable standalone program infrastructure
# ======================================================================

#undef RVTEST_RV64U
#define RVTEST_RV64U  .macro init; .endm
#undef RVTEST_RV64UF
#define RVTEST_RV64UF .macro init; .endm
#undef RVTEST_RV32U
#define RVTEST_RV32U  .macro init; .endm
#undef RVTEST_RV32UF
#define RVTEST_RV32UF .macro init; RVTEST_FP_ENABLE; .endm

#undef RVTEST_PASS
#define RVTEST_PASS   j op_pass
#undef RVTEST_FAIL
#define RVTEST_FAIL   j op_fail

#undef RVTEST_CODE_BEGIN
#define RVTEST_CODE_BEGIN
#undef RVTEST_CODE_END
#define RVTEST_CODE_END
#undef RVTEST_DATA_BEGIN
#define RVTEST_DATA_BEGIN
#undef RVTEST_DATA_END
#define RVTEST_DATA_END

#undef TEST_PASSFAIL
#define TEST_PASSFAIL \
    bne x0, TESTNUM, pass; \
    j fail

# ======================================================================
# Boot Code (.text.init)
# ======================================================================
.section .text.init
.align 2
.globl _start
_start:
    j reset_vector

.align 2
trap_vector:
    csrr t5, mcause
    li t6, CAUSE_MACHINE_ECALL
    beq t5, t6, .Lecall_ret

    li t0, CAUSE_MISALIGNED_LOAD
    beq t5, t0, .Lmisaligned_load

    li t0, CAUSE_MISALIGNED_STORE
    beq t5, t0, .Lmisaligned_store

    # Unexpected exception
    li TESTNUM, 0xDEAD
    ori TESTNUM, TESTNUM, 1

.Lecall_ret:
    la t0, _test_ctx_sp
    lw sp, 0(t0)
    la t0, _test_ctx_ra
    lw ra, 0(t0)
    ret
ecall_handle:
    j .Lecall_ret

.Lmisaligned_load:
    mv a4, a5
    csrr t0, mepc
    addi t0, t0, 4
    csrw mepc, t0
    mret

.Lmisaligned_store:
    csrw mepc, x15
    mret

reset_vector:
    INIT_XREG
    csrr a0, mhartid
1:  bnez a0, 1b
    INIT_RNMI
    INIT_SATP
    INIT_PMP
    DELEGATE_NO_TRAPS
    la t0, trap_vector
    csrw mtvec, t0
    li a0, MSTATUS_FS & (MSTATUS_FS >> 1)
    csrs mstatus, a0
    csrwi fcsr, 0
    li TESTNUM, 0
    la sp, _stack_top
    call main

#define REGBYTES (4)
.macro ISA_SAVE_CONTEXT
    addi sp,sp,-40*REGBYTES
    sw x4 , 3  *REGBYTES(sp)
    sw x5 , 4  *REGBYTES(sp)
    sw x6 , 5  *REGBYTES(sp)
    sw x7 , 6  *REGBYTES(sp)
    sw x8 , 7  *REGBYTES(sp)
    sw x9 , 8  *REGBYTES(sp)
    sw x10, 9  *REGBYTES(sp)
    sw x11, 10 *REGBYTES(sp)
    sw x12, 11 *REGBYTES(sp)
    sw x13, 12 *REGBYTES(sp)
    sw x14, 13 *REGBYTES(sp)
    sw x15, 14 *REGBYTES(sp)
    sw x16, 15 *REGBYTES(sp)
    sw x17, 16 *REGBYTES(sp)
    sw x18, 17 *REGBYTES(sp)
    sw x19, 18 *REGBYTES(sp)
    sw x20, 19 *REGBYTES(sp)
    sw x21, 20 *REGBYTES(sp)
    sw x22, 21 *REGBYTES(sp)
    sw x23, 22 *REGBYTES(sp)
    sw x24, 23 *REGBYTES(sp)
    sw x25, 24 *REGBYTES(sp)
    sw x26, 25 *REGBYTES(sp)
    sw x27, 26 *REGBYTES(sp)
    sw x28, 27 *REGBYTES(sp)
    sw x29, 28 *REGBYTES(sp)
    sw x30, 29 *REGBYTES(sp)
    sw x31, 30 *REGBYTES(sp)
.endm

.macro ISA_RESTORE_CONTEXT
    lw x4 , 3  *REGBYTES(sp)
    lw x5 , 4  *REGBYTES(sp)
    lw x6 , 5  *REGBYTES(sp)
    lw x7 , 6  *REGBYTES(sp)
    lw x8 , 7  *REGBYTES(sp)
    lw x9 , 8  *REGBYTES(sp)
    lw x10, 9  *REGBYTES(sp)
    lw x11, 10 *REGBYTES(sp)
    lw x12, 11 *REGBYTES(sp)
    lw x13, 12 *REGBYTES(sp)
    lw x14, 13 *REGBYTES(sp)
    lw x15, 14 *REGBYTES(sp)
    lw x16, 15 *REGBYTES(sp)
    lw x17, 16 *REGBYTES(sp)
    lw x18, 17 *REGBYTES(sp)
    lw x19, 18 *REGBYTES(sp)
    lw x20, 19 *REGBYTES(sp)
    lw x21, 20 *REGBYTES(sp)
    lw x22, 21 *REGBYTES(sp)
    lw x23, 22 *REGBYTES(sp)
    lw x24, 23 *REGBYTES(sp)
    lw x25, 24 *REGBYTES(sp)
    lw x26, 25 *REGBYTES(sp)
    lw x27, 26 *REGBYTES(sp)
    lw x28, 27 *REGBYTES(sp)
    lw x29, 28 *REGBYTES(sp)
    lw x30, 29 *REGBYTES(sp)
    lw x31, 30 *REGBYTES(sp)

    addi sp, sp, 40*REGBYTES
.endm

.globl op_fail
.globl op_pass
op_fail:
    la t0, _test_ctx_sp
    lw sp, 0(t0)
    la t0, _test_ctx_ra
    lw ra, 0(t0)
    ISA_RESTORE_CONTEXT
    sll a0, TESTNUM, 1
    ori a0, a0, 1
    ret

op_pass:
    la t0, _test_ctx_sp
    lw sp, 0(t0)
    la t0, _test_ctx_ra
    lw ra, 0(t0)
    ISA_RESTORE_CONTEXT
    li a0, 0
    ret
# ======================================================================
# Test Framework (.text)
# ======================================================================
.section .text
.align 2

fail:
    RVTEST_FAIL

pass:
    RVTEST_PASS

.macro FUNC_PROLOGUE name
    .text
    .align 2
    .globl \name
\name:
    ISA_SAVE_CONTEXT
    la t0, _test_ctx_ra
    sw ra, 0(t0)
    la t0, _test_ctx_sp
    sw sp, 0(t0)
    li TESTNUM, 0
.endm

.macro FUNC_EPILOGUE
    li a0, -1
    ret
.endm

# ======================================================================
# rv32ui: Base Integer Tests
# ======================================================================

FUNC_PROLOGUE test_rv32ui_add
  TEST_RR_OP(1001,  add, 0x00000000, 0x00000000, 0x00000000 );
  TEST_RR_OP(1002,  add, 0x00000002, 0x00000001, 0x00000001 );
  TEST_RR_OP(1003,  add, 0x0000000a, 0x00000003, 0x00000007 );
  TEST_RR_OP(1004,  add, 0xffffffffffff8000, 0x0000000000000000, 0xffffffffffff8000 );
  TEST_RR_OP(1005,  add, 0xffffffff80000000, 0xffffffff80000000, 0x00000000 );
  TEST_RR_OP(1006,  add, 0xffffffff7fff8000, 0xffffffff80000000, 0xffffffffffff8000 );
  TEST_RR_OP(1007,  add, 0x0000000000007fff, 0x0000000000000000, 0x0000000000007fff );
  TEST_RR_OP(1008,  add, 0x000000007fffffff, 0x000000007fffffff, 0x0000000000000000 );
  TEST_RR_OP(1009, add, 0x0000000080007ffe, 0x000000007fffffff, 0x0000000000007fff );
  TEST_RR_OP(1010, add, 0xffffffff80007fff, 0xffffffff80000000, 0x0000000000007fff );
  TEST_RR_OP(1011, add, 0x000000007fff7fff, 0x000000007fffffff, 0xffffffffffff8000 );
  TEST_RR_OP(1012, add, 0xffffffffffffffff, 0x0000000000000000, 0xffffffffffffffff );
  TEST_RR_OP(1013, add, 0x0000000000000000, 0xffffffffffffffff, 0x0000000000000001 );
  TEST_RR_OP(1014, add, 0xfffffffffffffffe, 0xffffffffffffffff, 0xffffffffffffffff );
  TEST_RR_OP(1015, add, 0x0000000080000000, 0x0000000000000001, 0x000000007fffffff );
  TEST_RR_SRC1_EQ_DEST(1016, add, 24, 13, 11 );
  TEST_RR_SRC2_EQ_DEST(1017, add, 25, 14, 11 );
  TEST_RR_SRC12_EQ_DEST(1018, add, 26, 13 );
  TEST_RR_DEST_BYPASS(1019, 0, add, 24, 13, 11 );
  TEST_RR_DEST_BYPASS(1020, 1, add, 25, 14, 11 );
  TEST_RR_DEST_BYPASS(1021, 2, add, 26, 15, 11 );
  TEST_RR_SRC12_BYPASS(1022, 0, 0, add, 24, 13, 11 );
  TEST_RR_SRC12_BYPASS(1023, 0, 1, add, 25, 14, 11 );
  TEST_RR_SRC12_BYPASS(1024, 0, 2, add, 26, 15, 11 );
  TEST_RR_SRC12_BYPASS(1025, 1, 0, add, 24, 13, 11 );
  TEST_RR_SRC12_BYPASS(1026, 1, 1, add, 25, 14, 11 );
  TEST_RR_SRC12_BYPASS(1027, 2, 0, add, 26, 15, 11 );
  TEST_RR_SRC21_BYPASS(1028, 0, 0, add, 24, 13, 11 );
  TEST_RR_SRC21_BYPASS(1029, 0, 1, add, 25, 14, 11 );
  TEST_RR_SRC21_BYPASS(1030, 0, 2, add, 26, 15, 11 );
  TEST_RR_SRC21_BYPASS(1031, 1, 0, add, 24, 13, 11 );
  TEST_RR_SRC21_BYPASS(1032, 1, 1, add, 25, 14, 11 );
  TEST_RR_SRC21_BYPASS(1033, 2, 0, add, 26, 15, 11 );
  TEST_RR_ZEROSRC1(1034, add, 15, 15 );
  TEST_RR_ZEROSRC2(1035, add, 32, 32 );
  TEST_RR_ZEROSRC12(1036, add, 0 );
  TEST_RR_ZERODEST(1037, add, 16, 30 );
  TEST_PASSFAIL
FUNC_EPILOGUE

FUNC_PROLOGUE test_rv32ui_addi
  TEST_IMM_OP(2001,  addi, 0x00000000, 0x00000000, 0x000 );
  TEST_IMM_OP(2002,  addi, 0x00000002, 0x00000001, 0x001 );
  TEST_IMM_OP(2003,  addi, 0x0000000a, 0x00000003, 0x007 );
  TEST_IMM_OP(2004,  addi, 0xfffffffffffff800, 0x0000000000000000, 0x800 );
  TEST_IMM_OP(2005,  addi, 0xffffffff80000000, 0xffffffff80000000, 0x000 );
  TEST_IMM_OP(2006,  addi, 0xffffffff7ffff800, 0xffffffff80000000, 0x800 );
  TEST_IMM_OP(2007,  addi, 0x00000000000007ff, 0x0000000000000000, 0x7ff );
  TEST_IMM_OP(2008,  addi, 0x000000007fffffff, 0x000000007fffffff, 0x000 );
  TEST_IMM_OP(2009, addi, 0x00000000800007fe, 0x000000007fffffff, 0x7ff );
  TEST_IMM_OP(2010, addi, 0xffffffff800007ff, 0xffffffff80000000, 0x7ff );
  TEST_IMM_OP(2011, addi, 0x000000007ffff7ff, 0x000000007fffffff, 0x800 );
  TEST_IMM_OP(2012, addi, 0xffffffffffffffff, 0x0000000000000000, 0xfff );
  TEST_IMM_OP(2013, addi, 0x0000000000000000, 0xffffffffffffffff, 0x001 );
  TEST_IMM_OP(2014, addi, 0xfffffffffffffffe, 0xffffffffffffffff, 0xfff );
  TEST_IMM_OP(2015, addi, 0x000000000000800, 0x0000000000000001, 0x7ff );
  TEST_IMM_SRC1_EQ_DEST(2016, addi, 24, 13, 11 );
  TEST_IMM_DEST_BYPASS(2017, 0, addi, 24, 13, 11 );
  TEST_IMM_DEST_BYPASS(2018, 1, addi, 23, 13, 10 );
  TEST_IMM_DEST_BYPASS(2019, 2, addi, 22, 13,  9 );
  TEST_IMM_SRC1_BYPASS(2020, 0, addi, 24, 13, 11 );
  TEST_IMM_SRC1_BYPASS(2021, 1, addi, 23, 13, 10 );
  TEST_IMM_SRC1_BYPASS(2022, 2, addi, 22, 13,  9 );
  TEST_IMM_ZEROSRC1(2023, addi, 32, 32 );
  TEST_IMM_ZERODEST(2024, addi, 33, 50 );
  TEST_PASSFAIL
FUNC_EPILOGUE

FUNC_PROLOGUE test_rv32ui_and
  TEST_RR_OP(3001, and, 0x0f000f00, 0xff00ff00, 0x0ff00ff0 );
  TEST_RR_OP(3002, and, 0x00000000, 0x0ff00ff0, 0xf00ff00f );
  TEST_RR_OP(3003, and, 0x000f000f, 0x00ff00ff, 0x0f0f0f0f );
  TEST_RR_OP(3004, and, 0xf000f000, 0xf00ff00f, 0xf0f0f0f0 );
  TEST_RR_SRC1_EQ_DEST(3005, and, 0x0f000f00, 0xff00ff00, 0x0ff00ff0 );
  TEST_RR_SRC2_EQ_DEST(3006, and, 0x00000000, 0x0ff00ff0, 0xf00ff00f );
  TEST_RR_SRC12_EQ_DEST(3007, and, 0xff00ff00, 0xff00ff00 );
  TEST_RR_DEST_BYPASS(3008,  0, and, 0x0f000f00, 0xff00ff00, 0x0ff00ff0 );
  TEST_RR_DEST_BYPASS(3009, 1, and, 0x00000000, 0x0ff00ff0, 0xf00ff00f );
  TEST_RR_DEST_BYPASS(3010, 2, and, 0x000f000f, 0x00ff00ff, 0x0f0f0f0f );
  TEST_RR_SRC12_BYPASS(3011, 0, 0, and, 0x0f000f00, 0xff00ff00, 0x0ff00ff0 );
  TEST_RR_SRC12_BYPASS(3012, 0, 1, and, 0x00000000, 0x0ff00ff0, 0xf00ff00f );
  TEST_RR_SRC12_BYPASS(3013, 0, 2, and, 0x000f000f, 0x00ff00ff, 0x0f0f0f0f );
  TEST_RR_SRC12_BYPASS(3014, 1, 0, and, 0x0f000f00, 0xff00ff00, 0x0ff00ff0 );
  TEST_RR_SRC12_BYPASS(3015, 1, 1, and, 0x00000000, 0x0ff00ff0, 0xf00ff00f );
  TEST_RR_SRC12_BYPASS(3016, 2, 0, and, 0x000f000f, 0x00ff00ff, 0x0f0f0f0f );
  TEST_RR_SRC21_BYPASS(3017, 0, 0, and, 0x0f000f00, 0xff00ff00, 0x0ff00ff0 );
  TEST_RR_SRC21_BYPASS(3018, 0, 1, and, 0x00000000, 0x0ff00ff0, 0xf00ff00f );
  TEST_RR_SRC21_BYPASS(3019, 0, 2, and, 0x000f000f, 0x00ff00ff, 0x0f0f0f0f );
  TEST_RR_SRC21_BYPASS(3020, 1, 0, and, 0x0f000f00, 0xff00ff00, 0x0ff00ff0 );
  TEST_RR_SRC21_BYPASS(3021, 1, 1, and, 0x00000000, 0x0ff00ff0, 0xf00ff00f );
  TEST_RR_SRC21_BYPASS(3022, 2, 0, and, 0x000f000f, 0x00ff00ff, 0x0f0f0f0f );
  TEST_RR_ZEROSRC1(3023, and, 0, 0xff00ff00 );
  TEST_RR_ZEROSRC2(3024, and, 0, 0x00ff00ff );
  TEST_RR_ZEROSRC12(3025, and, 0 );
  TEST_RR_ZERODEST(3026, and, 0x11111111, 0x22222222 );
  TEST_PASSFAIL
FUNC_EPILOGUE

FUNC_PROLOGUE test_rv32ui_andi
  TEST_IMM_OP(4001, andi, 0xff00ff00, 0xff00ff00, 0xf0f );
  TEST_IMM_OP(4002, andi, 0x000000f0, 0x0ff00ff0, 0x0f0 );
  TEST_IMM_OP(4003, andi, 0x0000000f, 0x00ff00ff, 0x70f );
  TEST_IMM_OP(4004, andi, 0x00000000, 0xf00ff00f, 0x0f0 );
  TEST_IMM_SRC1_EQ_DEST(4005, andi, 0x00000000, 0xff00ff00, 0x0f0 );
  TEST_IMM_DEST_BYPASS(4006,  0, andi, 0xff00ff00, 0xff00ff00, 0xf0f );
  TEST_IMM_DEST_BYPASS(4007,  1, andi, 0x000000f0, 0x0ff00ff0, 0x0f0 );
  TEST_IMM_DEST_BYPASS(4008,  2, andi, 0x0000000f, 0x00ff00ff, 0x70f );
  TEST_IMM_SRC1_BYPASS(4009, 0, andi, 0xff00ff00, 0xff00ff00, 0xf0f );
  TEST_IMM_SRC1_BYPASS(4010, 1, andi, 0x000000f0, 0x0ff00ff0, 0x0f0 );
  TEST_IMM_SRC1_BYPASS(4011, 2, andi, 0x0000000f, 0x00ff00ff, 0x70f );
  TEST_IMM_ZEROSRC1(4012, andi, 0, 0x0f0 );
  TEST_IMM_ZERODEST(4013, andi, 0x00ff00ff, 0x70f );
  TEST_PASSFAIL
FUNC_EPILOGUE

FUNC_PROLOGUE test_rv32ui_auipc
  # auipc 结果与 PC 相关：用数值局部标签回引做差，期望值 = imm<<12（与位置无关）
  TEST_CASE(5001, x1, 0x00000000, 5001: auipc x1, 0x00000; la x2, 5001b; sub x1, x1, x2; );
  TEST_CASE(5002, x1, 0xfffff000, 5002: auipc x1, 0xfffff; la x2, 5002b; sub x1, x1, x2; );
  TEST_CASE(5003, x1, 0x00001000, 5003: auipc x1, 0x00001; la x2, 5003b; sub x1, x1, x2; );
  TEST_CASE(5004, x1, 0xffffe000, 5004: auipc x1, 0xffffe; la x2, 5004b; sub x1, x1, x2; );
  TEST_CASE(5005, x1, 0x00002000, 5005: auipc x1, 0x00002; la x2, 5005b; sub x1, x1, x2; );
  TEST_PASSFAIL
FUNC_EPILOGUE
# ======================================================================
# rv32ui: beq
# ======================================================================
FUNC_PROLOGUE test_rv32ui_beq
  TEST_BR2_OP_TAKEN(6001, beq,  0,  0 );
  TEST_BR2_OP_TAKEN(6002, beq,  1,  1 );
  TEST_BR2_OP_TAKEN(6003, beq, -1, -1 );

  TEST_BR2_OP_NOTTAKEN(6004, beq,  0,  1 );
  TEST_BR2_OP_NOTTAKEN(6005, beq,  1,  0 );
  TEST_BR2_OP_NOTTAKEN(6006, beq, -1,  1 );
  TEST_BR2_OP_NOTTAKEN(6007, beq,  1, -1 );

  TEST_BR2_SRC12_BYPASS(6008,  0, 0, beq, 0, -1 );
  TEST_BR2_SRC12_BYPASS(6009, 0, 1, beq, 0, -1 );
  TEST_BR2_SRC12_BYPASS(6010, 0, 2, beq, 0, -1 );
  TEST_BR2_SRC12_BYPASS(6011, 1, 0, beq, 0, -1 );
  TEST_BR2_SRC12_BYPASS(6012, 1, 1, beq, 0, -1 );
  TEST_BR2_SRC12_BYPASS(6013, 2, 0, beq, 0, -1 );

  TEST_BR2_SRC12_BYPASS(6014, 0, 0, beq, 0, -1 );
  TEST_BR2_SRC12_BYPASS(6015, 0, 1, beq, 0, -1 );
  TEST_BR2_SRC12_BYPASS(6016, 0, 2, beq, 0, -1 );
  TEST_BR2_SRC12_BYPASS(6017, 1, 0, beq, 0, -1 );
  TEST_BR2_SRC12_BYPASS(6018, 1, 1, beq, 0, -1 );
  TEST_BR2_SRC12_BYPASS(6019, 2, 0, beq, 0, -1 );

  TEST_CASE(6020, x1, 3, \
    li  x1, 1; \
    beq x0, x0, 1f; \
    addi x1, x1, 1; \
    addi x1, x1, 1; \
    addi x1, x1, 1; \
    addi x1, x1, 1; \
1:  addi x1, x1, 1; \
    addi x1, x1, 1; \
  )

  TEST_PASSFAIL
FUNC_EPILOGUE


# ======================================================================
# rv32ui: bge
# ======================================================================
FUNC_PROLOGUE test_rv32ui_bge

  #-------------------------------------------------------------
  # Branch tests
  #-------------------------------------------------------------

  # Each test checks both forward and backward branches

  TEST_BR2_OP_TAKEN(7001, bge,  0,  0 );
  TEST_BR2_OP_TAKEN(7002, bge,  1,  1 );
  TEST_BR2_OP_TAKEN(7003, bge, -1, -1 );
  TEST_BR2_OP_TAKEN(7004, bge,  1,  0 );
  TEST_BR2_OP_TAKEN(7005, bge,  1, -1 );
  TEST_BR2_OP_TAKEN(7006, bge, -1, -2 );

  TEST_BR2_OP_NOTTAKEN(7007, bge,  0,  1 );
  TEST_BR2_OP_NOTTAKEN(7008, bge, -1,  1 );
  TEST_BR2_OP_NOTTAKEN(7009, bge, -2, -1 );
  TEST_BR2_OP_NOTTAKEN(7010, bge, -2,  1 );

  #-------------------------------------------------------------
  # Bypassing tests
  #-------------------------------------------------------------

  TEST_BR2_SRC12_BYPASS(7011, 0, 0, bge, -1, 0 );
  TEST_BR2_SRC12_BYPASS(7012, 0, 1, bge, -1, 0 );
  TEST_BR2_SRC12_BYPASS(7013, 0, 2, bge, -1, 0 );
  TEST_BR2_SRC12_BYPASS(7014, 1, 0, bge, -1, 0 );
  TEST_BR2_SRC12_BYPASS(7015, 1, 1, bge, -1, 0 );
  TEST_BR2_SRC12_BYPASS(7016, 2, 0, bge, -1, 0 );

  TEST_BR2_SRC12_BYPASS(7017, 0, 0, bge, -1, 0 );
  TEST_BR2_SRC12_BYPASS(7018, 0, 1, bge, -1, 0 );
  TEST_BR2_SRC12_BYPASS(7019, 0, 2, bge, -1, 0 );
  TEST_BR2_SRC12_BYPASS(7020, 1, 0, bge, -1, 0 );
  TEST_BR2_SRC12_BYPASS(7021, 1, 1, bge, -1, 0 );
  TEST_BR2_SRC12_BYPASS(7022, 2, 0, bge, -1, 0 );

  #-------------------------------------------------------------
  # Test delay slot instructions not executed nor bypassed
  #-------------------------------------------------------------

  TEST_CASE(7023, x1, 3, \
    li  x1, 1; \
    bge x1, x0, 1f; \
    addi x1, x1, 1; \
    addi x1, x1, 1; \
    addi x1, x1, 1; \
    addi x1, x1, 1; \
1:  addi x1, x1, 1; \
    addi x1, x1, 1; \
  )

  TEST_PASSFAIL
FUNC_EPILOGUE


# ======================================================================
# rv32ui: bgeu
# ======================================================================
FUNC_PROLOGUE test_rv32ui_bgeu

  #-------------------------------------------------------------
  # Branch tests
  #-------------------------------------------------------------

  # Each test checks both forward and backward branches

  TEST_BR2_OP_TAKEN(8001, bgeu, 0x00000000, 0x00000000 );
  TEST_BR2_OP_TAKEN(8002, bgeu, 0x00000001, 0x00000001 );
  TEST_BR2_OP_TAKEN(8003, bgeu, 0xffffffff, 0xffffffff );
  TEST_BR2_OP_TAKEN(8004, bgeu, 0x00000001, 0x00000000 );
  TEST_BR2_OP_TAKEN(8005, bgeu, 0xffffffff, 0xfffffffe );
  TEST_BR2_OP_TAKEN(8006, bgeu, 0xffffffff, 0x00000000 );

  TEST_BR2_OP_NOTTAKEN(8007, bgeu, 0x00000000, 0x00000001 );
  TEST_BR2_OP_NOTTAKEN(8008, bgeu, 0xfffffffe, 0xffffffff );
  TEST_BR2_OP_NOTTAKEN(8009, bgeu, 0x00000000, 0xffffffff );
  TEST_BR2_OP_NOTTAKEN(8010, bgeu, 0x7fffffff, 0x80000000 );

  #-------------------------------------------------------------
  # Bypassing tests
  #-------------------------------------------------------------

  TEST_BR2_SRC12_BYPASS(8011, 0, 0, bgeu, 0xefffffff, 0xf0000000 );
  TEST_BR2_SRC12_BYPASS(8012, 0, 1, bgeu, 0xefffffff, 0xf0000000 );
  TEST_BR2_SRC12_BYPASS(8013, 0, 2, bgeu, 0xefffffff, 0xf0000000 );
  TEST_BR2_SRC12_BYPASS(8014, 1, 0, bgeu, 0xefffffff, 0xf0000000 );
  TEST_BR2_SRC12_BYPASS(8015, 1, 1, bgeu, 0xefffffff, 0xf0000000 );
  TEST_BR2_SRC12_BYPASS(8016, 2, 0, bgeu, 0xefffffff, 0xf0000000 );

  TEST_BR2_SRC12_BYPASS(8017, 0, 0, bgeu, 0xefffffff, 0xf0000000 );
  TEST_BR2_SRC12_BYPASS(8018, 0, 1, bgeu, 0xefffffff, 0xf0000000 );
  TEST_BR2_SRC12_BYPASS(8019, 0, 2, bgeu, 0xefffffff, 0xf0000000 );
  TEST_BR2_SRC12_BYPASS(8020, 1, 0, bgeu, 0xefffffff, 0xf0000000 );
  TEST_BR2_SRC12_BYPASS(8021, 1, 1, bgeu, 0xefffffff, 0xf0000000 );
  TEST_BR2_SRC12_BYPASS(8022, 2, 0, bgeu, 0xefffffff, 0xf0000000 );

  #-------------------------------------------------------------
  # Test delay slot instructions not executed nor bypassed
  #-------------------------------------------------------------

  TEST_CASE(8023, x1, 3, \
    li  x1, 1; \
    bgeu x1, x0, 1f; \
    addi x1, x1, 1; \
    addi x1, x1, 1; \
    addi x1, x1, 1; \
    addi x1, x1, 1; \
1:  addi x1, x1, 1; \
    addi x1, x1, 1; \
  )

  TEST_PASSFAIL
FUNC_EPILOGUE


# ======================================================================
# rv32ui: blt
# ======================================================================
FUNC_PROLOGUE test_rv32ui_blt

  #-------------------------------------------------------------
  # Branch tests
  #-------------------------------------------------------------

  # Each test checks both forward and backward branches

  TEST_BR2_OP_TAKEN(9001, blt,  0,  1 );
  TEST_BR2_OP_TAKEN(9002, blt, -1,  1 );
  TEST_BR2_OP_TAKEN(9003, blt, -2, -1 );

  TEST_BR2_OP_NOTTAKEN(9004, blt,  1,  0 );
  TEST_BR2_OP_NOTTAKEN(9005, blt,  1, -1 );
  TEST_BR2_OP_NOTTAKEN(9006, blt, -1, -2 );
  TEST_BR2_OP_NOTTAKEN(9007, blt,  1, -2 );

  #-------------------------------------------------------------
  # Bypassing tests
  #-------------------------------------------------------------

  TEST_BR2_SRC12_BYPASS(9008,  0, 0, blt, 0, -1 );
  TEST_BR2_SRC12_BYPASS(9009, 0, 1, blt, 0, -1 );
  TEST_BR2_SRC12_BYPASS(9010, 0, 2, blt, 0, -1 );
  TEST_BR2_SRC12_BYPASS(9011, 1, 0, blt, 0, -1 );
  TEST_BR2_SRC12_BYPASS(9012, 1, 1, blt, 0, -1 );
  TEST_BR2_SRC12_BYPASS(9013, 2, 0, blt, 0, -1 );

  TEST_BR2_SRC12_BYPASS(9014, 0, 0, blt, 0, -1 );
  TEST_BR2_SRC12_BYPASS(9015, 0, 1, blt, 0, -1 );
  TEST_BR2_SRC12_BYPASS(9016, 0, 2, blt, 0, -1 );
  TEST_BR2_SRC12_BYPASS(9017, 1, 0, blt, 0, -1 );
  TEST_BR2_SRC12_BYPASS(9018, 1, 1, blt, 0, -1 );
  TEST_BR2_SRC12_BYPASS(9019, 2, 0, blt, 0, -1 );

  #-------------------------------------------------------------
  # Test delay slot instructions not executed nor bypassed
  #-------------------------------------------------------------

  TEST_CASE(9020, x1, 3, \
    li  x1, 1; \
    blt x0, x1, 1f; \
    addi x1, x1, 1; \
    addi x1, x1, 1; \
    addi x1, x1, 1; \
    addi x1, x1, 1; \
1:  addi x1, x1, 1; \
    addi x1, x1, 1; \
  )

  TEST_PASSFAIL
FUNC_EPILOGUE


# ======================================================================
# rv32ui: bltu
# ======================================================================
FUNC_PROLOGUE test_rv32ui_bltu

  #-------------------------------------------------------------
  # Branch tests
  #-------------------------------------------------------------

  # Each test checks both forward and backward branches

  TEST_BR2_OP_TAKEN(10001, bltu, 0x00000000, 0x00000001 );
  TEST_BR2_OP_TAKEN(10002, bltu, 0xfffffffe, 0xffffffff );
  TEST_BR2_OP_TAKEN(10003, bltu, 0x00000000, 0xffffffff );

  TEST_BR2_OP_NOTTAKEN(10004, bltu, 0x00000001, 0x00000000 );
  TEST_BR2_OP_NOTTAKEN(10005, bltu, 0xffffffff, 0xfffffffe );
  TEST_BR2_OP_NOTTAKEN(10006, bltu, 0xffffffff, 0x00000000 );
  TEST_BR2_OP_NOTTAKEN(10007, bltu, 0x80000000, 0x7fffffff );

  #-------------------------------------------------------------
  # Bypassing tests
  #-------------------------------------------------------------

  TEST_BR2_SRC12_BYPASS(10008,  0, 0, bltu, 0xf0000000, 0xefffffff );
  TEST_BR2_SRC12_BYPASS(10009, 0, 1, bltu, 0xf0000000, 0xefffffff );
  TEST_BR2_SRC12_BYPASS(10010, 0, 2, bltu, 0xf0000000, 0xefffffff );
  TEST_BR2_SRC12_BYPASS(10011, 1, 0, bltu, 0xf0000000, 0xefffffff );
  TEST_BR2_SRC12_BYPASS(10012, 1, 1, bltu, 0xf0000000, 0xefffffff );
  TEST_BR2_SRC12_BYPASS(10013, 2, 0, bltu, 0xf0000000, 0xefffffff );

  TEST_BR2_SRC12_BYPASS(10014, 0, 0, bltu, 0xf0000000, 0xefffffff );
  TEST_BR2_SRC12_BYPASS(10015, 0, 1, bltu, 0xf0000000, 0xefffffff );
  TEST_BR2_SRC12_BYPASS(10016, 0, 2, bltu, 0xf0000000, 0xefffffff );
  TEST_BR2_SRC12_BYPASS(10017, 1, 0, bltu, 0xf0000000, 0xefffffff );
  TEST_BR2_SRC12_BYPASS(10018, 1, 1, bltu, 0xf0000000, 0xefffffff );
  TEST_BR2_SRC12_BYPASS(10019, 2, 0, bltu, 0xf0000000, 0xefffffff );

  #-------------------------------------------------------------
  # Test delay slot instructions not executed nor bypassed
  #-------------------------------------------------------------

  TEST_CASE(10020, x1, 3, \
    li  x1, 1; \
    bltu x0, x1, 1f; \
    addi x1, x1, 1; \
    addi x1, x1, 1; \
    addi x1, x1, 1; \
    addi x1, x1, 1; \
1:  addi x1, x1, 1; \
    addi x1, x1, 1; \
  )

  TEST_PASSFAIL
FUNC_EPILOGUE


# ======================================================================
# rv32ui: bne
# ======================================================================
FUNC_PROLOGUE test_rv32ui_bne

  #-------------------------------------------------------------
  # Branch tests
  #-------------------------------------------------------------

  # Each test checks both forward and backward branches

  TEST_BR2_OP_TAKEN(11001, bne,  0,  1 );
  TEST_BR2_OP_TAKEN(11002, bne,  1,  0 );
  TEST_BR2_OP_TAKEN(11003, bne, -1,  1 );
  TEST_BR2_OP_TAKEN(11004, bne,  1, -1 );

  TEST_BR2_OP_NOTTAKEN(11005, bne,  0,  0 );
  TEST_BR2_OP_NOTTAKEN(11006, bne,  1,  1 );
  TEST_BR2_OP_NOTTAKEN(11007, bne, -1, -1 );

  #-------------------------------------------------------------
  # Bypassing tests
  #-------------------------------------------------------------

  TEST_BR2_SRC12_BYPASS(11008,  0, 0, bne, 0, 0 );
  TEST_BR2_SRC12_BYPASS(11009, 0, 1, bne, 0, 0 );
  TEST_BR2_SRC12_BYPASS(11010, 0, 2, bne, 0, 0 );
  TEST_BR2_SRC12_BYPASS(11011, 1, 0, bne, 0, 0 );
  TEST_BR2_SRC12_BYPASS(11012, 1, 1, bne, 0, 0 );
  TEST_BR2_SRC12_BYPASS(11013, 2, 0, bne, 0, 0 );

  TEST_BR2_SRC12_BYPASS(11014, 0, 0, bne, 0, 0 );
  TEST_BR2_SRC12_BYPASS(11015, 0, 1, bne, 0, 0 );
  TEST_BR2_SRC12_BYPASS(11016, 0, 2, bne, 0, 0 );
  TEST_BR2_SRC12_BYPASS(11017, 1, 0, bne, 0, 0 );
  TEST_BR2_SRC12_BYPASS(11018, 1, 1, bne, 0, 0 );
  TEST_BR2_SRC12_BYPASS(11019, 2, 0, bne, 0, 0 );

  #-------------------------------------------------------------
  # Test delay slot instructions not executed nor bypassed
  #-------------------------------------------------------------

  TEST_CASE(11020, x1, 3, \
    li  x1, 1; \
    bne x1, x0, 1f; \
    addi x1, x1, 1; \
    addi x1, x1, 1; \
    addi x1, x1, 1; \
    addi x1, x1, 1; \
1:  addi x1, x1, 1; \
    addi x1, x1, 1; \
  )

  TEST_PASSFAIL
FUNC_EPILOGUE


# ======================================================================
# rv32ui: jal
# ======================================================================
FUNC_PROLOGUE test_rv32ui_jal

  #-------------------------------------------------------------
  # Test 2: Basic test
  #-------------------------------------------------------------

test_12002:
  li  TESTNUM, 12002
  li  ra, 0

  jal x4, target_jal
linkaddr_jal:
  nop
  nop

  j fail

target_jal:
  la  x2, linkaddr_jal
  bne x2, x4, fail

  #-------------------------------------------------------------
  # Test delay slot instructions not executed nor bypassed
  #-------------------------------------------------------------

  TEST_CASE(12001, ra, 3, \
    li  ra, 1; \
    jal x0, 1f; \
    addi ra, ra, 1; \
    addi ra, ra, 1; \
    addi ra, ra, 1; \
    addi ra, ra, 1; \
1:  addi ra, ra, 1; \
    addi ra, ra, 1; \
  )

  TEST_PASSFAIL
FUNC_EPILOGUE


# ======================================================================
# rv32ui: jalr
# ======================================================================
FUNC_PROLOGUE test_rv32ui_jalr

  #-------------------------------------------------------------
  # Test 2: Basic test
  #-------------------------------------------------------------

test_13001:
  li  TESTNUM, 13001
  li  t0, 0
  la  t1, target_jalr

  jalr t0, t1, 0
linkaddr_jalr:
  j fail

target_jalr:
  la  t1, linkaddr_jalr
  bne t0, t1, fail

  #-------------------------------------------------------------
  # Test 3: Basic test2, rs = rd
  #-------------------------------------------------------------

test_13002:
  li  TESTNUM, 13002
  la  t0, target_3_jalr

  jalr t0, t0, 0
linkaddr_3_jalr:
  j fail

target_3_jalr:
  la  t1, linkaddr_3_jalr
  bne t0, t1, fail

  #-------------------------------------------------------------
  # Bypassing tests
  #-------------------------------------------------------------

  TEST_JALR_SRC1_BYPASS(13003, 0, jalr );
  TEST_JALR_SRC1_BYPASS(13004, 1, jalr );
  TEST_JALR_SRC1_BYPASS(13005, 2, jalr );

  #-------------------------------------------------------------
  # Test delay slot instructions not executed nor bypassed
  #-------------------------------------------------------------

  .option push
  .align 2
  .option norvc
  TEST_CASE(13006, t0, 4, \
    li  t0, 1; \
    la  t1, 1f; \
    jr  t1, -4; \
    addi t0, t0, 1; \
    addi t0, t0, 1; \
    addi t0, t0, 1; \
    addi t0, t0, 1; \
1:  addi t0, t0, 1; \
    addi t0, t0, 1; \
  )
  .option pop

  TEST_PASSFAIL
FUNC_EPILOGUE


# ======================================================================
# rv32ui: lb
# ======================================================================
FUNC_PROLOGUE test_rv32ui_lb

  #-------------------------------------------------------------
  # Basic tests
  #-------------------------------------------------------------

  TEST_LD_OP(14001, lb, 0xffffffffffffffff, 0,  tdat );
  TEST_LD_OP(14002, lb, 0x0000000000000000, 1,  tdat );
  TEST_LD_OP(14003, lb, 0xffffffffffffffff, 2,  tdat );
  TEST_LD_OP(14004, lb, 0x0000000000000000, 3, tdat );

  # Test with negative offset

  TEST_LD_OP(14005, lb, 0x000000000000000f, -3, tdat4 );
  TEST_LD_OP(14006, lb, 0xfffffffffffffff0, -2,  tdat4 );
  TEST_LD_OP(14007, lb, 0x000000000000000f, -1,  tdat4 );
  TEST_LD_OP(14008, lb, 0x000000000000000f, 0,   tdat4 );

  # Test with a negative base

  TEST_CASE(14009, x5, 0xffffffffffffffff, \
    la  x1, tdat; \
    addi x1, x1, -32; \
    lb x5, 32(x1); \
  )

  # Test with unaligned base

  TEST_CASE(14010, x5, 0x0000000000000000, \
    la  x1, tdat; \
    addi x1, x1, -6; \
    lb x5, 7(x1); \
  )

  #-------------------------------------------------------------
  # Bypassing tests
  #-------------------------------------------------------------

  TEST_LD_DEST_BYPASS(14011, 0, lb, 0xffffffffffffffff, 1, tdat2 );
  TEST_LD_DEST_BYPASS(14012, 1, lb, 0x000000000000000f, 1, tdat3 );
  TEST_LD_DEST_BYPASS(14013, 2, lb, 0x0000000000000000, 1, tdat1 );

  TEST_LD_SRC1_BYPASS(14014, 0, lb, 0xffffffffffffffff, 1, tdat2 );
  TEST_LD_SRC1_BYPASS(14015, 1, lb, 0x000000000000000f, 1, tdat3 );
  TEST_LD_SRC1_BYPASS(14016, 2, lb, 0x0000000000000000, 1, tdat1 );

  #-------------------------------------------------------------
  # Test write-after-write hazard
  #-------------------------------------------------------------

  TEST_CASE(14017, x2, 2, \
    la  x5, tdat; \
    lb  x2, 0(x5); \
    li  x2, 2; \
  )

  TEST_CASE(14018, x2, 2, \
    la  x5, tdat; \
    lb  x2, 0(x5); \
    nop; \
    li  x2, 2; \
  )

  TEST_PASSFAIL
FUNC_EPILOGUE


# ======================================================================
# rv32ui: lbu
# ======================================================================
FUNC_PROLOGUE test_rv32ui_lbu

  #-------------------------------------------------------------
  # Basic tests
  #-------------------------------------------------------------

  TEST_LD_OP(15001, lbu, 0x00000000000000ff, 0,  tdat );
  TEST_LD_OP(15002, lbu, 0x0000000000000000, 1,  tdat );
  TEST_LD_OP(15003, lbu, 0x00000000000000ff, 2,  tdat );
  TEST_LD_OP(15004, lbu, 0x0000000000000000, 3,  tdat );

  # Test with negative offset

  TEST_LD_OP(15005, lbu, 0x000000000000000f, -3, tdat4 );
  TEST_LD_OP(15006, lbu, 0x00000000000000f0, -2,  tdat4 );
  TEST_LD_OP(15007, lbu, 0x000000000000000f, -1,  tdat4 );
  TEST_LD_OP(15008, lbu, 0x000000000000000f, 0,   tdat4 );

  # Test with a negative base

  TEST_CASE(15009, x5, 0x00000000000000ff, \
    la  x1, tdat; \
    addi x1, x1, -32; \
    lbu x5, 32(x1); \
  )

  # Test with unaligned base

  TEST_CASE(15010, x5, 0x0000000000000000, \
    la  x1, tdat; \
    addi x1, x1, -6; \
    lbu x5, 7(x1); \
  )

  #-------------------------------------------------------------
  # Bypassing tests
  #-------------------------------------------------------------

  TEST_LD_DEST_BYPASS(15011, 0, lbu, 0x00000000000000ff, 1, tdat2 );
  TEST_LD_DEST_BYPASS(15012, 1, lbu, 0x000000000000000f, 1, tdat3 );
  TEST_LD_DEST_BYPASS(15013, 2, lbu, 0x0000000000000000, 1, tdat1 );

  TEST_LD_SRC1_BYPASS(15014, 0, lbu, 0x00000000000000ff, 1, tdat2 );
  TEST_LD_SRC1_BYPASS(15015, 1, lbu, 0x000000000000000f, 1, tdat3 );
  TEST_LD_SRC1_BYPASS(15016, 2, lbu, 0x0000000000000000, 1, tdat1 );

  #-------------------------------------------------------------
  # Test write-after-write hazard
  #-------------------------------------------------------------

  TEST_CASE(15017, x2, 2, \
    la  x5, tdat; \
    lbu  x2, 0(x5); \
    li  x2, 2; \
  )

  TEST_CASE(15018, x2, 2, \
    la  x5, tdat; \
    lbu  x2, 0(x5); \
    nop; \
    li  x2, 2; \
  )

  TEST_PASSFAIL
FUNC_EPILOGUE


# ======================================================================
# rv32ui: lh
# ======================================================================
FUNC_PROLOGUE test_rv32ui_lh

  #-------------------------------------------------------------
  # Basic tests
  #-------------------------------------------------------------

  TEST_LD_OP(16001, lh, 0x00000000000000ff, 0,  tdat );
  TEST_LD_OP(16002, lh, 0x00000000000000ff, 2,  tdat );
  TEST_LD_OP(16003, lh, 0xffffffffffffff00, 4,  tdat );
  TEST_LD_OP(16004, lh, 0xffffffffffffff00, 6,  tdat );

  # Test with negative offset

  TEST_LD_OP(16005, lh, 0xffffffffffffff00, -6,  tdat4 );
  TEST_LD_OP(16006, lh, 0x0000000000000ff0, -4,  tdat4 );
  TEST_LD_OP(16007, lh, 0x0000000000000ff0, -2,  tdat4 );
  TEST_LD_OP(16008, lh, 0xfffffffffffff00f,  0, tdat4 );

  # Test with a negative base

  TEST_CASE(16009, x5, 0x00000000000000ff, \
    la  x1, tdat; \
    addi x1, x1, -32; \
    lh x5, 32(x1); \
  )

  # Test with unaligned base

  TEST_CASE(16010, x5, 0x00000000000000ff, \
    la  x1, tdat; \
    addi x1, x1, -5; \
    lh x5, 7(x1); \
  )

  #-------------------------------------------------------------
  # Bypassing tests
  #-------------------------------------------------------------

  TEST_LD_DEST_BYPASS(16011, 0, lh, 0xffffffffffffff00, 2, tdat2 );
  TEST_LD_DEST_BYPASS(16012, 1, lh, 0x0000000000000ff0, 2, tdat3 );
  TEST_LD_DEST_BYPASS(16013, 2, lh, 0x00000000000000ff, 2, tdat1 );

  TEST_LD_SRC1_BYPASS(16014, 0, lh, 0xffffffffffffff00, 2, tdat2 );
  TEST_LD_SRC1_BYPASS(16015, 1, lh, 0x0000000000000ff0, 2, tdat3 );
  TEST_LD_SRC1_BYPASS(16016, 2, lh, 0x00000000000000ff, 2, tdat1 );

  #-------------------------------------------------------------
  # Test write-after-write hazard
  #-------------------------------------------------------------

  TEST_CASE(16017, x2, 2, \
    la  x5, tdat; \
    lh  x2, 0(x5); \
    li  x2, 2; \
  )

  TEST_CASE(16018, x2, 2, \
    la  x5, tdat; \
    lh  x2, 0(x5); \
    nop; \
    li  x2, 2; \
  )

  TEST_PASSFAIL
FUNC_EPILOGUE


# ======================================================================
# rv32ui: lhu
# ======================================================================
FUNC_PROLOGUE test_rv32ui_lhu

  #-------------------------------------------------------------
  # Basic tests
  #-------------------------------------------------------------

  TEST_LD_OP(17001, lhu, 0x00000000000000ff, 0,  tdat );
  TEST_LD_OP(17002, lhu, 0x00000000000000ff, 2,  tdat );
  TEST_LD_OP(17003, lhu, 0x000000000000ff00, 4,  tdat );
  TEST_LD_OP(17004, lhu, 0x000000000000ff00, 6,  tdat );

  # Test with negative offset

  TEST_LD_OP(17005, lhu, 0x000000000000ff00, -6,  tdat4 );
  TEST_LD_OP(17006, lhu, 0x0000000000000ff0, -4,  tdat4 );
  TEST_LD_OP(17007, lhu, 0x0000000000000ff0, -2,  tdat4 );
  TEST_LD_OP(17008, lhu, 0x000000000000f00f,  0,  tdat4 );

  # Test with a negative base

  TEST_CASE(17009, x5, 0x00000000000000ff, \
    la  x1, tdat; \
    addi x1, x1, -32; \
    lhu x5, 32(x1); \
  )

  # Test with unaligned base

  TEST_CASE(17010, x5, 0x00000000000000ff, \
    la  x1, tdat; \
    addi x1, x1, -5; \
    lhu x5, 7(x1); \
  )

  #-------------------------------------------------------------
  # Bypassing tests
  #-------------------------------------------------------------

  TEST_LD_DEST_BYPASS(17011, 0, lhu, 0x000000000000ff00, 2, tdat2 );
  TEST_LD_DEST_BYPASS(17012, 1, lhu, 0x0000000000000ff0, 2, tdat3 );
  TEST_LD_DEST_BYPASS(17013, 2, lhu, 0x00000000000000ff, 2, tdat1 );

  TEST_LD_SRC1_BYPASS(17014, 0, lhu, 0x000000000000ff00, 2, tdat2 );
  TEST_LD_SRC1_BYPASS(17015, 1, lhu, 0x0000000000000ff0, 2, tdat3 );
  TEST_LD_SRC1_BYPASS(17016, 2, lhu, 0x00000000000000ff, 2, tdat1 );

  #-------------------------------------------------------------
  # Test write-after-write hazard
  #-------------------------------------------------------------

  TEST_CASE(17017, x2, 2, \
    la  x5, tdat; \
    lhu  x2, 0(x5); \
    li  x2, 2; \
  )

  TEST_CASE(17018, x2, 2, \
    la  x5, tdat; \
    lhu  x2, 0(x5); \
    nop; \
    li  x2, 2; \
  )

  TEST_PASSFAIL
FUNC_EPILOGUE


# ======================================================================
# rv32ui: lui
# ======================================================================
FUNC_PROLOGUE test_rv32ui_lui

  #-------------------------------------------------------------
  # Basic tests
  #-------------------------------------------------------------

  TEST_CASE(18001, x1, 0x0000000000000000, lui x1, 0x00000 );
  TEST_CASE(18002, x1, 0xfffffffffffff800, lui x1, 0xfffff;sra x1,x1,1);
  TEST_CASE(18003, x1, 0x00000000000007ff, lui x1, 0x7ffff;sra x1,x1,20);
  TEST_CASE(18004, x1, 0xfffffffffffff800, lui x1, 0x80000;sra x1,x1,20);

  TEST_CASE(18005, x0, 0, lui x0, 0x80000 );

  TEST_PASSFAIL
FUNC_EPILOGUE


# ======================================================================
# rv32ui: lw
# ======================================================================
FUNC_PROLOGUE test_rv32ui_lw

  #-------------------------------------------------------------
  # Basic tests
  #-------------------------------------------------------------

  TEST_LD_OP(19001, lw, 0x0000000000ff00ff, 0,  tdat );
  TEST_LD_OP(19002, lw, 0xffffffffff00ff00, 4,  tdat );
  TEST_LD_OP(19003, lw, 0x000000000ff00ff0, 8,  tdat );
  TEST_LD_OP(19004, lw, 0xfffffffff00ff00f, 12, tdat );

  # Test with negative offset

  TEST_LD_OP(19005, lw, 0x0000000000ff00ff, -12, tdat4 );
  TEST_LD_OP(19006, lw, 0xffffffffff00ff00, -8,  tdat4 );
  TEST_LD_OP(19007, lw, 0x000000000ff00ff0, -4,  tdat4 );
  TEST_LD_OP(19008, lw, 0xfffffffff00ff00f, 0,   tdat4 );

  # Test with a negative base

  TEST_CASE(19009, x5, 0x0000000000ff00ff, \
    la  x1, tdat; \
    addi x1, x1, -32; \
    lw x5, 32(x1); \
  )

  # Test with unaligned base

  TEST_CASE(19010, x5, 0xffffffffff00ff00, \
    la  x1, tdat; \
    addi x1, x1, -3; \
    lw x5, 7(x1); \
  )

  #-------------------------------------------------------------
  # Bypassing tests
  #-------------------------------------------------------------

  TEST_LD_DEST_BYPASS(19011, 0, lw, 0x000000000ff00ff0, 4, tdat2 );
  TEST_LD_DEST_BYPASS(19012, 1, lw, 0xfffffffff00ff00f, 4, tdat3 );
  TEST_LD_DEST_BYPASS(19013, 2, lw, 0xffffffffff00ff00, 4, tdat1 );

  TEST_LD_SRC1_BYPASS(19014, 0, lw, 0x000000000ff00ff0, 4, tdat2 );
  TEST_LD_SRC1_BYPASS(19015, 1, lw, 0xfffffffff00ff00f, 4, tdat3 );
  TEST_LD_SRC1_BYPASS(19016, 2, lw, 0xffffffffff00ff00, 4, tdat1 );

  #-------------------------------------------------------------
  # Test write-after-write hazard
  #-------------------------------------------------------------

  TEST_CASE(19017, x2, 2, \
    la  x5, tdat; \
    lw  x2, 0(x5); \
    li  x2, 2; \
  )

  TEST_CASE(19018, x2, 2, \
    la  x5, tdat; \
    lw  x2, 0(x5); \
    nop; \
    li  x2, 2; \
  )

  TEST_PASSFAIL
FUNC_EPILOGUE


# ======================================================================
# rv32ui: or
# ======================================================================
FUNC_PROLOGUE test_rv32ui_or

  #-------------------------------------------------------------
  # Logical tests
  #-------------------------------------------------------------

  TEST_RR_OP(20001, or, 0xff0fff0f, 0xff00ff00, 0x0f0f0f0f );
  TEST_RR_OP(20002, or, 0xfff0fff0, 0x0ff00ff0, 0xf0f0f0f0 );
  TEST_RR_OP(20003, or, 0x0fff0fff, 0x00ff00ff, 0x0f0f0f0f );
  TEST_RR_OP(20004, or, 0xf0fff0ff, 0xf00ff00f, 0xf0f0f0f0 );

  #-------------------------------------------------------------
  # Source/Destination tests
  #-------------------------------------------------------------

  TEST_RR_SRC1_EQ_DEST(20005, or, 0xff0fff0f, 0xff00ff00, 0x0f0f0f0f );
  TEST_RR_SRC2_EQ_DEST(20006, or, 0xff0fff0f, 0xff00ff00, 0x0f0f0f0f );
  TEST_RR_SRC12_EQ_DEST(20007, or, 0xff00ff00, 0xff00ff00 );

  #-------------------------------------------------------------
  # Bypassing tests
  #-------------------------------------------------------------

  TEST_RR_DEST_BYPASS(20008,  0, or, 0xff0fff0f, 0xff00ff00, 0x0f0f0f0f );
  TEST_RR_DEST_BYPASS(20009, 1, or, 0xfff0fff0, 0x0ff00ff0, 0xf0f0f0f0 );
  TEST_RR_DEST_BYPASS(20010, 2, or, 0x0fff0fff, 0x00ff00ff, 0x0f0f0f0f );

  TEST_RR_SRC12_BYPASS(20011, 0, 0, or, 0xff0fff0f, 0xff00ff00, 0x0f0f0f0f );
  TEST_RR_SRC12_BYPASS(20012, 0, 1, or, 0xfff0fff0, 0x0ff00ff0, 0xf0f0f0f0 );
  TEST_RR_SRC12_BYPASS(20013, 0, 2, or, 0x0fff0fff, 0x00ff00ff, 0x0f0f0f0f );
  TEST_RR_SRC12_BYPASS(20014, 1, 0, or, 0xff0fff0f, 0xff00ff00, 0x0f0f0f0f );
  TEST_RR_SRC12_BYPASS(20015, 1, 1, or, 0xfff0fff0, 0x0ff00ff0, 0xf0f0f0f0 );
  TEST_RR_SRC12_BYPASS(20016, 2, 0, or, 0x0fff0fff, 0x00ff00ff, 0x0f0f0f0f );

  TEST_RR_SRC21_BYPASS(20017, 0, 0, or, 0xff0fff0f, 0xff00ff00, 0x0f0f0f0f );
  TEST_RR_SRC21_BYPASS(20018, 0, 1, or, 0xfff0fff0, 0x0ff00ff0, 0xf0f0f0f0 );
  TEST_RR_SRC21_BYPASS(20019, 0, 2, or, 0x0fff0fff, 0x00ff00ff, 0x0f0f0f0f );
  TEST_RR_SRC21_BYPASS(20020, 1, 0, or, 0xff0fff0f, 0xff00ff00, 0x0f0f0f0f );
  TEST_RR_SRC21_BYPASS(20021, 1, 1, or, 0xfff0fff0, 0x0ff00ff0, 0xf0f0f0f0 );
  TEST_RR_SRC21_BYPASS(20022, 2, 0, or, 0x0fff0fff, 0x00ff00ff, 0x0f0f0f0f );

  TEST_RR_ZEROSRC1(20023, or, 0xff00ff00, 0xff00ff00 );
  TEST_RR_ZEROSRC2(20024, or, 0x00ff00ff, 0x00ff00ff );
  TEST_RR_ZEROSRC12(20025, or, 0 );
  TEST_RR_ZERODEST(20026, or, 0x11111111, 0x22222222 );

  TEST_PASSFAIL
FUNC_EPILOGUE


# ======================================================================
# rv32ui: ori
# ======================================================================
FUNC_PROLOGUE test_rv32ui_ori

  #-------------------------------------------------------------
  # Logical tests
  #-------------------------------------------------------------

  TEST_IMM_OP(21001, ori, 0xffffffffffffff0f, 0xffffffffff00ff00, 0xf0f );
  TEST_IMM_OP(21002, ori, 0x000000000ff00ff0, 0x000000000ff00ff0, 0x0f0 );
  TEST_IMM_OP(21003, ori, 0x0000000000ff07ff, 0x0000000000ff00ff, 0x70f );
  TEST_IMM_OP(21004, ori, 0xfffffffff00ff0ff, 0xfffffffff00ff00f, 0x0f0 );

  #-------------------------------------------------------------
  # Source/Destination tests
  #-------------------------------------------------------------

  TEST_IMM_SRC1_EQ_DEST(21005, ori, 0xff00fff0, 0xff00ff00, 0x0f0 );

  #-------------------------------------------------------------
  # Bypassing tests
  #-------------------------------------------------------------

  TEST_IMM_DEST_BYPASS(21006,  0, ori, 0x000000000ff00ff0, 0x000000000ff00ff0, 0x0f0 );
  TEST_IMM_DEST_BYPASS(21007,  1, ori, 0x0000000000ff07ff, 0x0000000000ff00ff, 0x70f );
  TEST_IMM_DEST_BYPASS(21008,  2, ori, 0xfffffffff00ff0ff, 0xfffffffff00ff00f, 0x0f0 );

  TEST_IMM_SRC1_BYPASS(21009, 0, ori, 0x000000000ff00ff0, 0x000000000ff00ff0, 0x0f0 );
  TEST_IMM_SRC1_BYPASS(21010, 1, ori, 0xffffffffffffffff, 0x0000000000ff00ff, 0xf0f );
  TEST_IMM_SRC1_BYPASS(21011, 2, ori, 0xfffffffff00ff0ff, 0xfffffffff00ff00f, 0x0f0 );

  TEST_IMM_ZEROSRC1(21012, ori, 0x0f0, 0x0f0 );
  TEST_IMM_ZERODEST(21013, ori, 0x00ff00ff, 0x70f );

  TEST_PASSFAIL
FUNC_EPILOGUE


# ======================================================================
# rv32ui: sb
# ======================================================================
FUNC_PROLOGUE test_rv32ui_sb

  #-------------------------------------------------------------
  # Basic tests
  #-------------------------------------------------------------

  TEST_ST_OP(22001, lb, sb, 0xffffffffffffffaa, 0, tdat5 );
  TEST_ST_OP(22002, lb, sb, 0x0000000000000000, 1, tdat5 );
  TEST_ST_OP(22003, lb, sb, 0xffffffffffffffa0, 2, tdat5 );
  TEST_ST_OP(22004, lb, sb, 0x000000000000000a, 3, tdat5 );

  # Test with negative offset

  TEST_ST_OP(22005, lb, sb, 0xffffffffffffffaa, -3, tdat8 );
  TEST_ST_OP(22006, lb, sb, 0x0000000000000000, -2, tdat8 );
  TEST_ST_OP(22007, lb, sb, 0xffffffffffffffa0, -1, tdat8 );
  TEST_ST_OP(22008, lb, sb, 0x000000000000000a, 0,  tdat8 );

  # Test with a negative base

  TEST_CASE(22009, x5, 0x78, \
    la  x1, tdat9; \
    li  x2, 0x12345678; \
    addi x4, x1, -32; \
    sb x2, 32(x4); \
    lb x5, 0(x1); \
  )

  # Test with unaligned base

  TEST_CASE(22010, x5, 0xffffffffffffff98, \
    la  x1, tdat9; \
    li  x2, 0xffffff98; \
    addi x1, x1, -6; \
    sb x2, 7(x1); \
    la  x4, tdat10; \
    lb x5, -3(x4); \
  )

  #-------------------------------------------------------------
  # Bypassing tests
  #-------------------------------------------------------------

  TEST_ST_SRC12_BYPASS(22011, 0, 0, lb, sb, 0xffffffffffffffdd, 0, tdat5 );
  TEST_ST_SRC12_BYPASS(22012, 0, 1, lb, sb, 0xffffffffffffffcd, 1, tdat5 );
  TEST_ST_SRC12_BYPASS(22013, 0, 2, lb, sb, 0xffffffffffffffcc, 2, tdat5 );
  TEST_ST_SRC12_BYPASS(22014, 1, 0, lb, sb, 0xffffffffffffffbc, 3, tdat5 );
  TEST_ST_SRC12_BYPASS(22015, 1, 1, lb, sb, 0xffffffffffffffbb, 4, tdat5 );
  TEST_ST_SRC12_BYPASS(22016, 2, 0, lb, sb, 0xffffffffffffffab, 5, tdat5 );

  TEST_ST_SRC21_BYPASS(22017, 0, 0, lb, sb, 0x33, 0, tdat5 );
  TEST_ST_SRC21_BYPASS(22018, 0, 1, lb, sb, 0x23, 1, tdat5 );
  TEST_ST_SRC21_BYPASS(22019, 0, 2, lb, sb, 0x22, 2, tdat5 );
  TEST_ST_SRC21_BYPASS(22020, 1, 0, lb, sb, 0x12, 3, tdat5 );
  TEST_ST_SRC21_BYPASS(22021, 1, 1, lb, sb, 0x11, 4, tdat5 );
  TEST_ST_SRC21_BYPASS(22022, 2, 0, lb, sb, 0x01, 5, tdat5 );

  li a0, 0xef
  la a1, tdat5
  sb a0, 3(a1)

  TEST_PASSFAIL
FUNC_EPILOGUE


# ======================================================================
# rv32ui: sh
# ======================================================================
FUNC_PROLOGUE test_rv32ui_sh

  #-------------------------------------------------------------
  # Basic tests
  #-------------------------------------------------------------

  TEST_ST_OP(23001, lh, sh, 0x00000000000000aa, 0, tdat5 );
  TEST_ST_OP(23002, lh, sh, 0xffffffffffffaa00, 2, tdat5 );
  TEST_ST_OP(23003, lh, sh, 0x0000000000000aa0, 4, tdat5 );
  TEST_ST_OP(23004, lh, sh, 0xffffffffffffa00a, 6, tdat5 );

  # Test with negative offset

  TEST_ST_OP(23005, lh, sh, 0x00000000000000aa, -6, tdat8 );
  TEST_ST_OP(23006, lh, sh, 0xffffffffffffaa00, -4, tdat8 );
  TEST_ST_OP(23007, lh, sh, 0x0000000000000aa0, -2, tdat8 );
  TEST_ST_OP(23008, lh, sh, 0xffffffffffffa00a, 0,  tdat8 );

  # Test with a negative base

  TEST_CASE(23009, x5, 0x5678, \
    la  x1, tdat9; \
    li  x2, 0x12345678; \
    addi x4, x1, -32; \
    sh x2, 32(x4); \
    lh x5, 0(x1); \
  )

  # Test with unaligned base

  TEST_CASE(23010, x5, 0x3098, \
    la  x1, tdat9; \
    li  x2, 0x00003098; \
    addi x1, x1, -5; \
    sh x2, 7(x1); \
    la  x4, tdat10; \
    lh x5, -2(x4); \
  )

  #-------------------------------------------------------------
  # Bypassing tests
  #-------------------------------------------------------------

  TEST_ST_SRC12_BYPASS(23011, 0, 0, lh, sh, 0xffffffffffffccdd, 0,  tdat5 );
  TEST_ST_SRC12_BYPASS(23012, 0, 1, lh, sh, 0xffffffffffffbccd, 2,  tdat5 );
  TEST_ST_SRC12_BYPASS(23013, 0, 2, lh, sh, 0xffffffffffffbbcc, 4,  tdat5 );
  TEST_ST_SRC12_BYPASS(23014, 1, 0, lh, sh, 0xffffffffffffabbc, 6,  tdat5 );
  TEST_ST_SRC12_BYPASS(23015, 1, 1, lh, sh, 0xffffffffffffaabb, 8,  tdat5 );
  TEST_ST_SRC12_BYPASS(23016, 2, 0, lh, sh, 0xffffffffffffdaab, 10, tdat5 );

  TEST_ST_SRC21_BYPASS(23017, 0, 0, lh, sh, 0x2233, 0,  tdat5 );
  TEST_ST_SRC21_BYPASS(23018, 0, 1, lh, sh, 0x1223, 2,  tdat5 );
  TEST_ST_SRC21_BYPASS(23019, 0, 2, lh, sh, 0x1122, 4,  tdat5 );
  TEST_ST_SRC21_BYPASS(23020, 1, 0, lh, sh, 0x0112, 6,  tdat5 );
  TEST_ST_SRC21_BYPASS(23021, 1, 1, lh, sh, 0x0011, 8,  tdat5 );
  TEST_ST_SRC21_BYPASS(23022, 2, 0, lh, sh, 0x3001, 10, tdat5 );

  li a0, 0xbeef
  la a1, tdat5
  sh a0, 6(a1)

  TEST_PASSFAIL
FUNC_EPILOGUE


# ======================================================================
# rv32ui: sll
# ======================================================================
FUNC_PROLOGUE test_rv32ui_sll

  #-------------------------------------------------------------
  # Arithmetic tests
  #-------------------------------------------------------------

  TEST_RR_OP(24001,  sll, 0x0000000000000001, 0x0000000000000001, 0  );
  TEST_RR_OP(24002,  sll, 0x0000000000000002, 0x0000000000000001, 1  );
  TEST_RR_OP(24003,  sll, 0x0000000000000080, 0x0000000000000001, 7  );
  TEST_RR_OP(24004,  sll, 0x0000000000004000, 0x0000000000000001, 14 );
  TEST_RR_OP(24005,  sll, 0x0000000080000000, 0x0000000000000001, 31 );

  TEST_RR_OP(24006,  sll, 0xffffffffffffffff, 0xffffffffffffffff, 0  );
  TEST_RR_OP(24007,  sll, 0xfffffffffffffffe, 0xffffffffffffffff, 1  );
  TEST_RR_OP(24008,  sll, 0xffffffffffffff80, 0xffffffffffffffff, 7  );
  TEST_RR_OP(24009, sll, 0xffffffffffffc000, 0xffffffffffffffff, 14 );
  TEST_RR_OP(24010, sll, 0xffffffff80000000, 0xffffffffffffffff, 31 );

  TEST_RR_OP(24011, sll, 0x0000000021212121, 0x0000000021212121, 0  );
  TEST_RR_OP(24012, sll, 0x0000000042424242, 0x0000000021212121, 1  );
  TEST_RR_OP(24013, sll, 0x0000001090909080, 0x0000000021212121, 7  );
  TEST_RR_OP(24014, sll, 0x0000084848484000, 0x0000000021212121, 14 );
  TEST_RR_OP(24015, sll, 0x1090909080000000, 0x0000000021212121, 31 );

  # Verify that shifts only use bottom six(rv64) or five(rv32) bits

  TEST_RR_OP(24016, sll, 0x0000000021212121, 0x0000000021212121, 0xffffffffffffffc0 );
  TEST_RR_OP(24017, sll, 0x0000000042424242, 0x0000000021212121, 0xffffffffffffffc1 );
  TEST_RR_OP(24018, sll, 0x0000001090909080, 0x0000000021212121, 0xffffffffffffffc7 );
  TEST_RR_OP(24019, sll, 0x0000084848484000, 0x0000000021212121, 0xffffffffffffffce );

  #-------------------------------------------------------------
  # Source/Destination tests
  #-------------------------------------------------------------

  TEST_RR_SRC1_EQ_DEST(24020, sll, 0x00000080, 0x00000001, 7  );
  TEST_RR_SRC2_EQ_DEST(24021, sll, 0x00004000, 0x00000001, 14 );
  TEST_RR_SRC12_EQ_DEST(24022, sll, 24, 3 );

  #-------------------------------------------------------------
  # Bypassing tests
  #-------------------------------------------------------------

  TEST_RR_DEST_BYPASS(24023, 0, sll, 0x0000000000000080, 0x0000000000000001, 7  );
  TEST_RR_DEST_BYPASS(24024, 1, sll, 0x0000000000004000, 0x0000000000000001, 14 );
  TEST_RR_DEST_BYPASS(24025, 2, sll, 0x0000000080000000, 0x0000000000000001, 31 );

  TEST_RR_SRC12_BYPASS(24026, 0, 0, sll, 0x0000000000000080, 0x0000000000000001, 7  );
  TEST_RR_SRC12_BYPASS(24027, 0, 1, sll, 0x0000000000004000, 0x0000000000000001, 14 );
  TEST_RR_SRC12_BYPASS(24028, 0, 2, sll, 0x0000000080000000, 0x0000000000000001, 31 );
  TEST_RR_SRC12_BYPASS(24029, 1, 0, sll, 0x0000000000000080, 0x0000000000000001, 7  );
  TEST_RR_SRC12_BYPASS(24030, 1, 1, sll, 0x0000000000004000, 0x0000000000000001, 14 );
  TEST_RR_SRC12_BYPASS(24031, 2, 0, sll, 0x0000000080000000, 0x0000000000000001, 31 );

  TEST_RR_SRC21_BYPASS(24032, 0, 0, sll, 0x0000000000000080, 0x0000000000000001, 7  );
  TEST_RR_SRC21_BYPASS(24033, 0, 1, sll, 0x0000000000004000, 0x0000000000000001, 14 );
  TEST_RR_SRC21_BYPASS(24034, 0, 2, sll, 0x0000000080000000, 0x0000000000000001, 31 );
  TEST_RR_SRC21_BYPASS(24035, 1, 0, sll, 0x0000000000000080, 0x0000000000000001, 7  );
  TEST_RR_SRC21_BYPASS(24036, 1, 1, sll, 0x0000000000004000, 0x0000000000000001, 14 );
  TEST_RR_SRC21_BYPASS(24037, 2, 0, sll, 0x0000000080000000, 0x0000000000000001, 31 );

  TEST_RR_ZEROSRC1(24038, sll, 0, 15 );
  TEST_RR_ZEROSRC2(24039, sll, 32, 32 );
  TEST_RR_ZEROSRC12(24040, sll, 0 );
  TEST_RR_ZERODEST(24041, sll, 1024, 2048 );

#if __riscv_xlen == 64
  TEST_RR_OP(24042, sll, 0x8000000000000000, 0x0000000021212121, 0xffffffffffffffff );
  TEST_RR_OP(24043, sll, 0x8000000000000000, 0x0000000000000001, 63 );
  TEST_RR_OP(24044, sll, 0xffffff8000000000, 0xffffffffffffffff, 39 );
  TEST_RR_OP(24045, sll, 0x0909080000000000, 0x0000000021212121, 43 );
#endif

  TEST_PASSFAIL
FUNC_EPILOGUE


# ======================================================================
# rv32ui: slli
# ======================================================================
FUNC_PROLOGUE test_rv32ui_slli

  #-------------------------------------------------------------
  # Arithmetic tests
  #-------------------------------------------------------------

  TEST_IMM_OP(25001,  slli, 0x0000000000000001, 0x0000000000000001, 0  );
  TEST_IMM_OP(25002,  slli, 0x0000000000000002, 0x0000000000000001, 1  );
  TEST_IMM_OP(25003,  slli, 0x0000000000000080, 0x0000000000000001, 7  );
  TEST_IMM_OP(25004,  slli, 0x0000000000004000, 0x0000000000000001, 14 );
  TEST_IMM_OP(25005,  slli, 0x0000000080000000, 0x0000000000000001, 31 );

  TEST_IMM_OP(25006,  slli, 0xffffffffffffffff, 0xffffffffffffffff, 0  );
  TEST_IMM_OP(25007,  slli, 0xfffffffffffffffe, 0xffffffffffffffff, 1  );
  TEST_IMM_OP(25008,  slli, 0xffffffffffffff80, 0xffffffffffffffff, 7  );
  TEST_IMM_OP(25009, slli, 0xffffffffffffc000, 0xffffffffffffffff, 14 );
  TEST_IMM_OP(25010, slli, 0xffffffff80000000, 0xffffffffffffffff, 31 );

  TEST_IMM_OP(25011, slli, 0x0000000021212121, 0x0000000021212121, 0  );
  TEST_IMM_OP(25012, slli, 0x0000000042424242, 0x0000000021212121, 1  );
  TEST_IMM_OP(25013, slli, 0x0000001090909080, 0x0000000021212121, 7  );
  TEST_IMM_OP(25014, slli, 0x0000084848484000, 0x0000000021212121, 14 );
  TEST_IMM_OP(25015, slli, 0x1090909080000000, 0x0000000021212121, 31 );

  #-------------------------------------------------------------
  # Source/Destination tests
  #-------------------------------------------------------------

  TEST_IMM_SRC1_EQ_DEST(25016, slli, 0x00000080, 0x00000001, 7 );

  #-------------------------------------------------------------
  # Bypassing tests
  #-------------------------------------------------------------

  TEST_IMM_DEST_BYPASS(25017, 0, slli, 0x0000000000000080, 0x0000000000000001, 7  );
  TEST_IMM_DEST_BYPASS(25018, 1, slli, 0x0000000000004000, 0x0000000000000001, 14 );
  TEST_IMM_DEST_BYPASS(25019, 2, slli, 0x0000000080000000, 0x0000000000000001, 31 );

  TEST_IMM_SRC1_BYPASS(25020, 0, slli, 0x0000000000000080, 0x0000000000000001, 7  );
  TEST_IMM_SRC1_BYPASS(25021, 1, slli, 0x0000000000004000, 0x0000000000000001, 14 );
  TEST_IMM_SRC1_BYPASS(25022, 2, slli, 0x0000000080000000, 0x0000000000000001, 31 );

  TEST_IMM_ZEROSRC1(25023, slli, 0, 31 );
  TEST_IMM_ZERODEST(25024, slli, 33, 20 );

#if __riscv_xlen == 64
  TEST_IMM_OP(25025, slli, 0x8000000000000000, 0x0000000000000001, 63 );
  TEST_IMM_OP(25026, slli, 0xffffff8000000000, 0xffffffffffffffff, 39 );
  TEST_IMM_OP(25027, slli, 0x0909080000000000, 0x0000000021212121, 43 );
#endif

  TEST_PASSFAIL
FUNC_EPILOGUE


# ======================================================================
# rv32ui: slt
# ======================================================================
FUNC_PROLOGUE test_rv32ui_slt

  #-------------------------------------------------------------
  # Arithmetic tests
  #-------------------------------------------------------------

  TEST_RR_OP(26001,  slt, 0, 0x0000000000000000, 0x0000000000000000 );
  TEST_RR_OP(26002,  slt, 0, 0x0000000000000001, 0x0000000000000001 );
  TEST_RR_OP(26003,  slt, 1, 0x0000000000000003, 0x0000000000000007 );
  TEST_RR_OP(26004,  slt, 0, 0x0000000000000007, 0x0000000000000003 );

  TEST_RR_OP(26005,  slt, 0, 0x0000000000000000, 0xffffffffffff8000 );
  TEST_RR_OP(26006,  slt, 1, 0xffffffff80000000, 0x0000000000000000 );
  TEST_RR_OP(26007,  slt, 1, 0xffffffff80000000, 0xffffffffffff8000 );

  TEST_RR_OP(26008,  slt, 1, 0x0000000000000000, 0x0000000000007fff );
  TEST_RR_OP(26009, slt, 0, 0x000000007fffffff, 0x0000000000000000 );
  TEST_RR_OP(26010, slt, 0, 0x000000007fffffff, 0x0000000000007fff );

  TEST_RR_OP(26011, slt, 1, 0xffffffff80000000, 0x0000000000007fff );
  TEST_RR_OP(26012, slt, 0, 0x000000007fffffff, 0xffffffffffff8000 );

  TEST_RR_OP(26013, slt, 0, 0x0000000000000000, 0xffffffffffffffff );
  TEST_RR_OP(26014, slt, 1, 0xffffffffffffffff, 0x0000000000000001 );
  TEST_RR_OP(26015, slt, 0, 0xffffffffffffffff, 0xffffffffffffffff );

  #-------------------------------------------------------------
  # Source/Destination tests
  #-------------------------------------------------------------

  TEST_RR_SRC1_EQ_DEST(26016, slt, 0, 14, 13 );
  TEST_RR_SRC2_EQ_DEST(26017, slt, 1, 11, 13 );
  TEST_RR_SRC12_EQ_DEST(26018, slt, 0, 13 );

  #-------------------------------------------------------------
  # Bypassing tests
  #-------------------------------------------------------------

  TEST_RR_DEST_BYPASS(26019, 0, slt, 1, 11, 13 );
  TEST_RR_DEST_BYPASS(26020, 1, slt, 0, 14, 13 );
  TEST_RR_DEST_BYPASS(26021, 2, slt, 1, 12, 13 );

  TEST_RR_SRC12_BYPASS(26022, 0, 0, slt, 0, 14, 13 );
  TEST_RR_SRC12_BYPASS(26023, 0, 1, slt, 1, 11, 13 );
  TEST_RR_SRC12_BYPASS(26024, 0, 2, slt, 0, 15, 13 );
  TEST_RR_SRC12_BYPASS(26025, 1, 0, slt, 1, 10, 13 );
  TEST_RR_SRC12_BYPASS(26026, 1, 1, slt, 0, 16, 13 );
  TEST_RR_SRC12_BYPASS(26027, 2, 0, slt, 1,  9, 13 );

  TEST_RR_SRC21_BYPASS(26028, 0, 0, slt, 0, 17, 13 );
  TEST_RR_SRC21_BYPASS(26029, 0, 1, slt, 1,  8, 13 );
  TEST_RR_SRC21_BYPASS(26030, 0, 2, slt, 0, 18, 13 );
  TEST_RR_SRC21_BYPASS(26031, 1, 0, slt, 1,  7, 13 );
  TEST_RR_SRC21_BYPASS(26032, 1, 1, slt, 0, 19, 13 );
  TEST_RR_SRC21_BYPASS(26033, 2, 0, slt, 1,  6, 13 );

  TEST_RR_ZEROSRC1(26034, slt, 0, -1 );
  TEST_RR_ZEROSRC2(26035, slt, 1, -1 );
  TEST_RR_ZEROSRC12(26036, slt, 0 );
  TEST_RR_ZERODEST(26037, slt, 16, 30 );

  TEST_PASSFAIL
FUNC_EPILOGUE


# ======================================================================
# rv32ui: slti
# ======================================================================
FUNC_PROLOGUE test_rv32ui_slti

  #-------------------------------------------------------------
  # Arithmetic tests
  #-------------------------------------------------------------

  TEST_IMM_OP(27001,  slti, 0, 0x0000000000000000, 0x000 );
  TEST_IMM_OP(27002,  slti, 0, 0x0000000000000001, 0x001 );
  TEST_IMM_OP(27003,  slti, 1, 0x0000000000000003, 0x007 );
  TEST_IMM_OP(27004,  slti, 0, 0x0000000000000007, 0x003 );

  TEST_IMM_OP(27005,  slti, 0, 0x0000000000000000, 0x800 );
  TEST_IMM_OP(27006,  slti, 1, 0xffffffff80000000, 0x000 );
  TEST_IMM_OP(27007,  slti, 1, 0xffffffff80000000, 0x800 );

  TEST_IMM_OP(27008,  slti, 1, 0x0000000000000000, 0x7ff );
  TEST_IMM_OP(27009, slti, 0, 0x000000007fffffff, 0x000 );
  TEST_IMM_OP(27010, slti, 0, 0x000000007fffffff, 0x7ff );

  TEST_IMM_OP(27011, slti, 1, 0xffffffff80000000, 0x7ff );
  TEST_IMM_OP(27012, slti, 0, 0x000000007fffffff, 0x800 );

  TEST_IMM_OP(27013, slti, 0, 0x0000000000000000, 0xfff );
  TEST_IMM_OP(27014, slti, 1, 0xffffffffffffffff, 0x001 );
  TEST_IMM_OP(27015, slti, 0, 0xffffffffffffffff, 0xfff );

  #-------------------------------------------------------------
  # Source/Destination tests
  #-------------------------------------------------------------

  TEST_IMM_SRC1_EQ_DEST(27016, slti, 1, 11, 13 );

  #-------------------------------------------------------------
  # Bypassing tests
  #-------------------------------------------------------------

  TEST_IMM_DEST_BYPASS(27017, 0, slti, 0, 15, 10 );
  TEST_IMM_DEST_BYPASS(27018, 1, slti, 1, 10, 16 );
  TEST_IMM_DEST_BYPASS(27019, 2, slti, 0, 16,  9 );

  TEST_IMM_SRC1_BYPASS(27020, 0, slti, 1, 11, 15 );
  TEST_IMM_SRC1_BYPASS(27021, 1, slti, 0, 17,  8 );
  TEST_IMM_SRC1_BYPASS(27022, 2, slti, 1, 12, 14 );

  TEST_IMM_ZEROSRC1(27023, slti, 0, 0xfff );
  TEST_IMM_ZERODEST(27024, slti, 0x00ff00ff, 0xfff );

  TEST_PASSFAIL
FUNC_EPILOGUE


# ======================================================================
# rv32ui: sltiu
# ======================================================================
FUNC_PROLOGUE test_rv32ui_sltiu

  #-------------------------------------------------------------
  # Arithmetic tests
  #-------------------------------------------------------------

  TEST_IMM_OP(28001,  sltiu, 0, 0x0000000000000000, 0x000 );
  TEST_IMM_OP(28002,  sltiu, 0, 0x0000000000000001, 0x001 );
  TEST_IMM_OP(28003,  sltiu, 1, 0x0000000000000003, 0x007 );
  TEST_IMM_OP(28004,  sltiu, 0, 0x0000000000000007, 0x003 );

  TEST_IMM_OP(28005,  sltiu, 1, 0x0000000000000000, 0x800 );
  TEST_IMM_OP(28006,  sltiu, 0, 0xffffffff80000000, 0x000 );
  TEST_IMM_OP(28007,  sltiu, 1, 0xffffffff80000000, 0x800 );

  TEST_IMM_OP(28008,  sltiu, 1, 0x0000000000000000, 0x7ff );
  TEST_IMM_OP(28009, sltiu, 0, 0x000000007fffffff, 0x000 );
  TEST_IMM_OP(28010, sltiu, 0, 0x000000007fffffff, 0x7ff );

  TEST_IMM_OP(28011, sltiu, 0, 0xffffffff80000000, 0x7ff );
  TEST_IMM_OP(28012, sltiu, 1, 0x000000007fffffff, 0x800 );

  TEST_IMM_OP(28013, sltiu, 1, 0x0000000000000000, 0xfff );
  TEST_IMM_OP(28014, sltiu, 0, 0xffffffffffffffff, 0x001 );
  TEST_IMM_OP(28015, sltiu, 0, 0xffffffffffffffff, 0xfff );

  #-------------------------------------------------------------
  # Source/Destination tests
  #-------------------------------------------------------------

  TEST_IMM_SRC1_EQ_DEST(28016, sltiu, 1, 11, 13 );

  #-------------------------------------------------------------
  # Bypassing tests
  #-------------------------------------------------------------

  TEST_IMM_DEST_BYPASS(28017, 0, sltiu, 0, 15, 10 );
  TEST_IMM_DEST_BYPASS(28018, 1, sltiu, 1, 10, 16 );
  TEST_IMM_DEST_BYPASS(28019, 2, sltiu, 0, 16,  9 );

  TEST_IMM_SRC1_BYPASS(28020, 0, sltiu, 1, 11, 15 );
  TEST_IMM_SRC1_BYPASS(28021, 1, sltiu, 0, 17,  8 );
  TEST_IMM_SRC1_BYPASS(28022, 2, sltiu, 1, 12, 14 );

  TEST_IMM_ZEROSRC1(28023, sltiu, 1, 0xfff );
  TEST_IMM_ZERODEST(28024, sltiu, 0x00ff00ff, 0xfff );

  TEST_PASSFAIL
FUNC_EPILOGUE


# ======================================================================
# rv32ui: sltu
# ======================================================================
FUNC_PROLOGUE test_rv32ui_sltu

  #-------------------------------------------------------------
  # Arithmetic tests
  #-------------------------------------------------------------

  TEST_RR_OP(29001,  sltu, 0, 0x00000000, 0x00000000 );
  TEST_RR_OP(29002,  sltu, 0, 0x00000001, 0x00000001 );
  TEST_RR_OP(29003,  sltu, 1, 0x00000003, 0x00000007 );
  TEST_RR_OP(29004,  sltu, 0, 0x00000007, 0x00000003 );

  TEST_RR_OP(29005,  sltu, 1, 0x00000000, 0xffff8000 );
  TEST_RR_OP(29006,  sltu, 0, 0x80000000, 0x00000000 );
  TEST_RR_OP(29007,  sltu, 1, 0x80000000, 0xffff8000 );

  TEST_RR_OP(29008,  sltu, 1, 0x00000000, 0x00007fff );
  TEST_RR_OP(29009, sltu, 0, 0x7fffffff, 0x00000000 );
  TEST_RR_OP(29010, sltu, 0, 0x7fffffff, 0x00007fff );

  TEST_RR_OP(29011, sltu, 0, 0x80000000, 0x00007fff );
  TEST_RR_OP(29012, sltu, 1, 0x7fffffff, 0xffff8000 );

  TEST_RR_OP(29013, sltu, 1, 0x00000000, 0xffffffff );
  TEST_RR_OP(29014, sltu, 0, 0xffffffff, 0x00000001 );
  TEST_RR_OP(29015, sltu, 0, 0xffffffff, 0xffffffff );

  #-------------------------------------------------------------
  # Source/Destination tests
  #-------------------------------------------------------------

  TEST_RR_SRC1_EQ_DEST(29016, sltu, 0, 14, 13 );
  TEST_RR_SRC2_EQ_DEST(29017, sltu, 1, 11, 13 );
  TEST_RR_SRC12_EQ_DEST(29018, sltu, 0, 13 );

  #-------------------------------------------------------------
  # Bypassing tests
  #-------------------------------------------------------------

  TEST_RR_DEST_BYPASS(29019, 0, sltu, 1, 11, 13 );
  TEST_RR_DEST_BYPASS(29020, 1, sltu, 0, 14, 13 );
  TEST_RR_DEST_BYPASS(29021, 2, sltu, 1, 12, 13 );

  TEST_RR_SRC12_BYPASS(29022, 0, 0, sltu, 0, 14, 13 );
  TEST_RR_SRC12_BYPASS(29023, 0, 1, sltu, 1, 11, 13 );
  TEST_RR_SRC12_BYPASS(29024, 0, 2, sltu, 0, 15, 13 );
  TEST_RR_SRC12_BYPASS(29025, 1, 0, sltu, 1, 10, 13 );
  TEST_RR_SRC12_BYPASS(29026, 1, 1, sltu, 0, 16, 13 );
  TEST_RR_SRC12_BYPASS(29027, 2, 0, sltu, 1,  9, 13 );

  TEST_RR_SRC21_BYPASS(29028, 0, 0, sltu, 0, 17, 13 );
  TEST_RR_SRC21_BYPASS(29029, 0, 1, sltu, 1,  8, 13 );
  TEST_RR_SRC21_BYPASS(29030, 0, 2, sltu, 0, 18, 13 );
  TEST_RR_SRC21_BYPASS(29031, 1, 0, sltu, 1,  7, 13 );
  TEST_RR_SRC21_BYPASS(29032, 1, 1, sltu, 0, 19, 13 );
  TEST_RR_SRC21_BYPASS(29033, 2, 0, sltu, 1,  6, 13 );

  TEST_RR_ZEROSRC1(29034, sltu, 1, -1 );
  TEST_RR_ZEROSRC2(29035, sltu, 0, -1 );
  TEST_RR_ZEROSRC12(29036, sltu, 0 );
  TEST_RR_ZERODEST(29037, sltu, 16, 30 );

  TEST_PASSFAIL
FUNC_EPILOGUE


# ======================================================================
# rv32ui: sra
# ======================================================================
FUNC_PROLOGUE test_rv32ui_sra

  #-------------------------------------------------------------
  # Arithmetic tests
  #-------------------------------------------------------------

  TEST_RR_OP(30001,  sra, 0xffffffff80000000, 0xffffffff80000000, 0  );
  TEST_RR_OP(30002,  sra, 0xffffffffc0000000, 0xffffffff80000000, 1  );
  TEST_RR_OP(30003,  sra, 0xffffffffff000000, 0xffffffff80000000, 7  );
  TEST_RR_OP(30004,  sra, 0xfffffffffffe0000, 0xffffffff80000000, 14 );
  TEST_RR_OP(30005,  sra, 0xffffffffffffffff, 0xffffffff80000001, 31 );

  TEST_RR_OP(30006,  sra, 0x000000007fffffff, 0x000000007fffffff, 0  );
  TEST_RR_OP(30007,  sra, 0x000000003fffffff, 0x000000007fffffff, 1  );
  TEST_RR_OP(30008,  sra, 0x0000000000ffffff, 0x000000007fffffff, 7  );
  TEST_RR_OP(30009, sra, 0x000000000001ffff, 0x000000007fffffff, 14 );
  TEST_RR_OP(30010, sra, 0x0000000000000000, 0x000000007fffffff, 31 );

  TEST_RR_OP(30011, sra, 0xffffffff81818181, 0xffffffff81818181, 0  );
  TEST_RR_OP(30012, sra, 0xffffffffc0c0c0c0, 0xffffffff81818181, 1  );
  TEST_RR_OP(30013, sra, 0xffffffffff030303, 0xffffffff81818181, 7  );
  TEST_RR_OP(30014, sra, 0xfffffffffffe0606, 0xffffffff81818181, 14 );
  TEST_RR_OP(30015, sra, 0xffffffffffffffff, 0xffffffff81818181, 31 );

  # Verify that shifts only use bottom six(rv64) or five(rv32) bits

  TEST_RR_OP(30016, sra, 0xffffffff81818181, 0xffffffff81818181, 0xffffffffffffffc0 );
  TEST_RR_OP(30017, sra, 0xffffffffc0c0c0c0, 0xffffffff81818181, 0xffffffffffffffc1 );
  TEST_RR_OP(30018, sra, 0xffffffffff030303, 0xffffffff81818181, 0xffffffffffffffc7 );
  TEST_RR_OP(30019, sra, 0xfffffffffffe0606, 0xffffffff81818181, 0xffffffffffffffce );
  TEST_RR_OP(30020, sra, 0xffffffffffffffff, 0xffffffff81818181, 0xffffffffffffffff );

  #-------------------------------------------------------------
  # Source/Destination tests
  #-------------------------------------------------------------

  TEST_RR_SRC1_EQ_DEST(30021, sra, 0xffffffffff000000, 0xffffffff80000000, 7  );
  TEST_RR_SRC2_EQ_DEST(30022, sra, 0xfffffffffffe0000, 0xffffffff80000000, 14 );
  TEST_RR_SRC12_EQ_DEST(30023, sra, 0, 7 );

  #-------------------------------------------------------------
  # Bypassing tests
  #-------------------------------------------------------------

  TEST_RR_DEST_BYPASS(30024, 0, sra, 0xffffffffff000000, 0xffffffff80000000, 7  );
  TEST_RR_DEST_BYPASS(30025, 1, sra, 0xfffffffffffe0000, 0xffffffff80000000, 14 );
  TEST_RR_DEST_BYPASS(30026, 2, sra, 0xffffffffffffffff, 0xffffffff80000000, 31 );

  TEST_RR_SRC12_BYPASS(30027, 0, 0, sra, 0xffffffffff000000, 0xffffffff80000000, 7  );
  TEST_RR_SRC12_BYPASS(30028, 0, 1, sra, 0xfffffffffffe0000, 0xffffffff80000000, 14 );
  TEST_RR_SRC12_BYPASS(30029, 0, 2, sra, 0xffffffffffffffff, 0xffffffff80000000, 31 );
  TEST_RR_SRC12_BYPASS(30030, 1, 0, sra, 0xffffffffff000000, 0xffffffff80000000, 7  );
  TEST_RR_SRC12_BYPASS(30031, 1, 1, sra, 0xfffffffffffe0000, 0xffffffff80000000, 14 );
  TEST_RR_SRC12_BYPASS(30032, 2, 0, sra, 0xffffffffffffffff, 0xffffffff80000000, 31 );

  TEST_RR_SRC21_BYPASS(30033, 0, 0, sra, 0xffffffffff000000, 0xffffffff80000000, 7  );
  TEST_RR_SRC21_BYPASS(30034, 0, 1, sra, 0xfffffffffffe0000, 0xffffffff80000000, 14 );
  TEST_RR_SRC21_BYPASS(30035, 0, 2, sra, 0xffffffffffffffff, 0xffffffff80000000, 31 );
  TEST_RR_SRC21_BYPASS(30036, 1, 0, sra, 0xffffffffff000000, 0xffffffff80000000, 7  );
  TEST_RR_SRC21_BYPASS(30037, 1, 1, sra, 0xfffffffffffe0000, 0xffffffff80000000, 14 );
  TEST_RR_SRC21_BYPASS(30038, 2, 0, sra, 0xffffffffffffffff, 0xffffffff80000000, 31 );

  TEST_RR_ZEROSRC1(30039, sra, 0, 15 );
  TEST_RR_ZEROSRC2(30040, sra, 32, 32 );
  TEST_RR_ZEROSRC12(30041, sra, 0 );
  TEST_RR_ZERODEST(30042, sra, 1024, 2048 );

  TEST_PASSFAIL
FUNC_EPILOGUE


# ======================================================================
# rv32ui: srai
# ======================================================================
FUNC_PROLOGUE test_rv32ui_srai

  #-------------------------------------------------------------
  # Arithmetic tests
  #-------------------------------------------------------------

  TEST_IMM_OP(31001,  srai, 0xffffff8000000000, 0xffffff8000000000, 0  );
  TEST_IMM_OP(31002,  srai, 0xffffffffc0000000, 0xffffffff80000000, 1  );
  TEST_IMM_OP(31003,  srai, 0xffffffffff000000, 0xffffffff80000000, 7  );
  TEST_IMM_OP(31004,  srai, 0xfffffffffffe0000, 0xffffffff80000000, 14 );
  TEST_IMM_OP(31005,  srai, 0xffffffffffffffff, 0xffffffff80000001, 31 );

  TEST_IMM_OP(31006,  srai, 0x000000007fffffff, 0x000000007fffffff, 0  );
  TEST_IMM_OP(31007,  srai, 0x000000003fffffff, 0x000000007fffffff, 1  );
  TEST_IMM_OP(31008,  srai, 0x0000000000ffffff, 0x000000007fffffff, 7  );
  TEST_IMM_OP(31009, srai, 0x000000000001ffff, 0x000000007fffffff, 14 );
  TEST_IMM_OP(31010, srai, 0x0000000000000000, 0x000000007fffffff, 31 );

  TEST_IMM_OP(31011, srai, 0xffffffff81818181, 0xffffffff81818181, 0  );
  TEST_IMM_OP(31012, srai, 0xffffffffc0c0c0c0, 0xffffffff81818181, 1  );
  TEST_IMM_OP(31013, srai, 0xffffffffff030303, 0xffffffff81818181, 7  );
  TEST_IMM_OP(31014, srai, 0xfffffffffffe0606, 0xffffffff81818181, 14 );
  TEST_IMM_OP(31015, srai, 0xffffffffffffffff, 0xffffffff81818181, 31 );

  #-------------------------------------------------------------
  # Source/Destination tests
  #-------------------------------------------------------------

  TEST_IMM_SRC1_EQ_DEST(31016, srai, 0xffffffffff000000, 0xffffffff80000000, 7 );

  #-------------------------------------------------------------
  # Bypassing tests
  #-------------------------------------------------------------

  TEST_IMM_DEST_BYPASS(31017, 0, srai, 0xffffffffff000000, 0xffffffff80000000, 7  );
  TEST_IMM_DEST_BYPASS(31018, 1, srai, 0xfffffffffffe0000, 0xffffffff80000000, 14 );
  TEST_IMM_DEST_BYPASS(31019, 2, srai, 0xffffffffffffffff, 0xffffffff80000001, 31 );

  TEST_IMM_SRC1_BYPASS(31020, 0, srai, 0xffffffffff000000, 0xffffffff80000000, 7 );
  TEST_IMM_SRC1_BYPASS(31021, 1, srai, 0xfffffffffffe0000, 0xffffffff80000000, 14 );
  TEST_IMM_SRC1_BYPASS(31022, 2, srai, 0xffffffffffffffff, 0xffffffff80000001, 31 );

  TEST_IMM_ZEROSRC1(31023, srai, 0, 4 );
  TEST_IMM_ZERODEST(31024, srai, 33, 10 );

  TEST_PASSFAIL
FUNC_EPILOGUE


# ======================================================================
# rv32ui: srl
# ======================================================================
FUNC_PROLOGUE test_rv32ui_srl

  #-------------------------------------------------------------
  # Arithmetic tests
  #-------------------------------------------------------------

#define TEST_SRL(n, v, a) \
  TEST_RR_OP(n, srl, ((v) & ((1 << (__riscv_xlen-1) << 1) - 1)) >> (a), v, a)

  TEST_SRL(32001,  0xffffffff80000000, 0  );
  TEST_SRL(32002,  0xffffffff80000000, 1  );
  TEST_SRL(32003,  0xffffffff80000000, 7  );
  TEST_SRL(32004,  0xffffffff80000000, 14 );
  TEST_SRL(32005,  0xffffffff80000001, 31 );

  TEST_SRL(32006,  0xffffffffffffffff, 0  );
  TEST_SRL(32007,  0xffffffffffffffff, 1  );
  TEST_SRL(32008,  0xffffffffffffffff, 7  );
  TEST_SRL(32009, 0xffffffffffffffff, 14 );
  TEST_SRL(32010, 0xffffffffffffffff, 31 );

  TEST_SRL(32011, 0x0000000021212121, 0  );
  TEST_SRL(32012, 0x0000000021212121, 1  );
  TEST_SRL(32013, 0x0000000021212121, 7  );
  TEST_SRL(32014, 0x0000000021212121, 14 );
  TEST_SRL(32015, 0x0000000021212121, 31 );

  # Verify that shifts only use bottom six(rv64) or five(rv32) bits

  TEST_RR_OP(32016, srl, 0x0000000021212121, 0x0000000021212121, 0xffffffffffffffc0 );
  TEST_RR_OP(32017, srl, 0x0000000010909090, 0x0000000021212121, 0xffffffffffffffc1 );
  TEST_RR_OP(32018, srl, 0x0000000000424242, 0x0000000021212121, 0xffffffffffffffc7 );
  TEST_RR_OP(32019, srl, 0x0000000000008484, 0x0000000021212121, 0xffffffffffffffce );
  TEST_RR_OP(32020, srl, 0x0000000000000000, 0x0000000021212121, 0xffffffffffffffff );

  #-------------------------------------------------------------
  # Source/Destination tests
  #-------------------------------------------------------------

  TEST_RR_SRC1_EQ_DEST(32021, srl, 0x01000000, 0x80000000, 7  );
  TEST_RR_SRC2_EQ_DEST(32022, srl, 0x00020000, 0x80000000, 14 );
  TEST_RR_SRC12_EQ_DEST(32023, srl, 0, 7 );

  #-------------------------------------------------------------
  # Bypassing tests
  #-------------------------------------------------------------

  TEST_RR_DEST_BYPASS(32024, 0, srl, 0x01000000, 0x80000000, 7  );
  TEST_RR_DEST_BYPASS(32025, 1, srl, 0x00020000, 0x80000000, 14 );
  TEST_RR_DEST_BYPASS(32026, 2, srl, 0x00000001, 0x80000000, 31 );

  TEST_RR_SRC12_BYPASS(32027, 0, 0, srl, 0x01000000, 0x80000000, 7  );
  TEST_RR_SRC12_BYPASS(32028, 0, 1, srl, 0x00020000, 0x80000000, 14 );
  TEST_RR_SRC12_BYPASS(32029, 0, 2, srl, 0x00000001, 0x80000000, 31 );
  TEST_RR_SRC12_BYPASS(32030, 1, 0, srl, 0x01000000, 0x80000000, 7  );
  TEST_RR_SRC12_BYPASS(32031, 1, 1, srl, 0x00020000, 0x80000000, 14 );
  TEST_RR_SRC12_BYPASS(32032, 2, 0, srl, 0x00000001, 0x80000000, 31 );

  TEST_RR_SRC21_BYPASS(32033, 0, 0, srl, 0x01000000, 0x80000000, 7  );
  TEST_RR_SRC21_BYPASS(32034, 0, 1, srl, 0x00020000, 0x80000000, 14 );
  TEST_RR_SRC21_BYPASS(32035, 0, 2, srl, 0x00000001, 0x80000000, 31 );
  TEST_RR_SRC21_BYPASS(32036, 1, 0, srl, 0x01000000, 0x80000000, 7  );
  TEST_RR_SRC21_BYPASS(32037, 1, 1, srl, 0x00020000, 0x80000000, 14 );
  TEST_RR_SRC21_BYPASS(32038, 2, 0, srl, 0x00000001, 0x80000000, 31 );

  TEST_RR_ZEROSRC1(32039, srl, 0, 15 );
  TEST_RR_ZEROSRC2(32040, srl, 32, 32 );
  TEST_RR_ZEROSRC12(32041, srl, 0 );
  TEST_RR_ZERODEST(32042, srl, 1024, 2048 );

  TEST_PASSFAIL
FUNC_EPILOGUE


# ======================================================================
# rv32ui: srli
# ======================================================================
FUNC_PROLOGUE test_rv32ui_srli

  #-------------------------------------------------------------
  # Arithmetic tests
  #-------------------------------------------------------------

#define TEST_SRLI(n, v, a) \
  TEST_IMM_OP(n, srli, ((v) & ((1 << (__riscv_xlen-1) << 1) - 1)) >> (a), v, a)

  TEST_SRLI(33001,  0xffffffff80000000, 0  );
  TEST_SRLI(33002,  0xffffffff80000000, 1  );
  TEST_SRLI(33003,  0xffffffff80000000, 7  );
  TEST_SRLI(33004,  0xffffffff80000000, 14 );
  TEST_SRLI(33005,  0xffffffff80000001, 31 );

  TEST_SRLI(33006,  0xffffffffffffffff, 0  );
  TEST_SRLI(33007,  0xffffffffffffffff, 1  );
  TEST_SRLI(33008,  0xffffffffffffffff, 7  );
  TEST_SRLI(33009, 0xffffffffffffffff, 14 );
  TEST_SRLI(33010, 0xffffffffffffffff, 31 );

  TEST_SRLI(33011, 0x0000000021212121, 0  );
  TEST_SRLI(33012, 0x0000000021212121, 1  );
  TEST_SRLI(33013, 0x0000000021212121, 7  );
  TEST_SRLI(33014, 0x0000000021212121, 14 );
  TEST_SRLI(33015, 0x0000000021212121, 31 );

  #-------------------------------------------------------------
  # Source/Destination tests
  #-------------------------------------------------------------

  TEST_IMM_SRC1_EQ_DEST(33016, srli, 0x01000000, 0x80000000, 7 );

  #-------------------------------------------------------------
  # Bypassing tests
  #-------------------------------------------------------------

  TEST_IMM_DEST_BYPASS(33017, 0, srli, 0x01000000, 0x80000000, 7  );
  TEST_IMM_DEST_BYPASS(33018, 1, srli, 0x00020000, 0x80000000, 14 );
  TEST_IMM_DEST_BYPASS(33019, 2, srli, 0x00000001, 0x80000001, 31 );

  TEST_IMM_SRC1_BYPASS(33020, 0, srli, 0x01000000, 0x80000000, 7  );
  TEST_IMM_SRC1_BYPASS(33021, 1, srli, 0x00020000, 0x80000000, 14 );
  TEST_IMM_SRC1_BYPASS(33022, 2, srli, 0x00000001, 0x80000001, 31 );

  TEST_IMM_ZEROSRC1(33023, srli, 0, 4 );
  TEST_IMM_ZERODEST(33024, srli, 33, 10 );

  TEST_PASSFAIL
FUNC_EPILOGUE


# ======================================================================
# rv32ui: sub
# ======================================================================
FUNC_PROLOGUE test_rv32ui_sub

  #-------------------------------------------------------------
  # Arithmetic tests
  #-------------------------------------------------------------

  TEST_RR_OP(34001,  sub, 0x0000000000000000, 0x0000000000000000, 0x0000000000000000 );
  TEST_RR_OP(34002,  sub, 0x0000000000000000, 0x0000000000000001, 0x0000000000000001 );
  TEST_RR_OP(34003,  sub, 0xfffffffffffffffc, 0x0000000000000003, 0x0000000000000007 );

  TEST_RR_OP(34004,  sub, 0x0000000000008000, 0x0000000000000000, 0xffffffffffff8000 );
  TEST_RR_OP(34005,  sub, 0xffffffff80000000, 0xffffffff80000000, 0x0000000000000000 );
  TEST_RR_OP(34006,  sub, 0xffffffff80008000, 0xffffffff80000000, 0xffffffffffff8000 );

  TEST_RR_OP(34007,  sub, 0xffffffffffff8001, 0x0000000000000000, 0x0000000000007fff );
  TEST_RR_OP(34008,  sub, 0x000000007fffffff, 0x000000007fffffff, 0x0000000000000000 );
  TEST_RR_OP(34009, sub, 0x000000007fff8000, 0x000000007fffffff, 0x0000000000007fff );

  TEST_RR_OP(34010, sub, 0xffffffff7fff8001, 0xffffffff80000000, 0x0000000000007fff );
  TEST_RR_OP(34011, sub, 0x0000000080007fff, 0x000000007fffffff, 0xffffffffffff8000 );

  TEST_RR_OP(34012, sub, 0x0000000000000001, 0x0000000000000000, 0xffffffffffffffff );
  TEST_RR_OP(34013, sub, 0xfffffffffffffffe, 0xffffffffffffffff, 0x0000000000000001 );
  TEST_RR_OP(34014, sub, 0x0000000000000000, 0xffffffffffffffff, 0xffffffffffffffff );

  #-------------------------------------------------------------
  # Source/Destination tests
  #-------------------------------------------------------------

  TEST_RR_SRC1_EQ_DEST(34015, sub, 2, 13, 11 );
  TEST_RR_SRC2_EQ_DEST(34016, sub, 3, 14, 11 );
  TEST_RR_SRC12_EQ_DEST(34017, sub, 0, 13 );

  #-------------------------------------------------------------
  # Bypassing tests
  #-------------------------------------------------------------

  TEST_RR_DEST_BYPASS(34018, 0, sub, 2, 13, 11 );
  TEST_RR_DEST_BYPASS(34019, 1, sub, 3, 14, 11 );
  TEST_RR_DEST_BYPASS(34020, 2, sub, 4, 15, 11 );

  TEST_RR_SRC12_BYPASS(34021, 0, 0, sub, 2, 13, 11 );
  TEST_RR_SRC12_BYPASS(34022, 0, 1, sub, 3, 14, 11 );
  TEST_RR_SRC12_BYPASS(34023, 0, 2, sub, 4, 15, 11 );
  TEST_RR_SRC12_BYPASS(34024, 1, 0, sub, 2, 13, 11 );
  TEST_RR_SRC12_BYPASS(34025, 1, 1, sub, 3, 14, 11 );
  TEST_RR_SRC12_BYPASS(34026, 2, 0, sub, 4, 15, 11 );

  TEST_RR_SRC21_BYPASS(34027, 0, 0, sub, 2, 13, 11 );
  TEST_RR_SRC21_BYPASS(34028, 0, 1, sub, 3, 14, 11 );
  TEST_RR_SRC21_BYPASS(34029, 0, 2, sub, 4, 15, 11 );
  TEST_RR_SRC21_BYPASS(34030, 1, 0, sub, 2, 13, 11 );
  TEST_RR_SRC21_BYPASS(34031, 1, 1, sub, 3, 14, 11 );
  TEST_RR_SRC21_BYPASS(34032, 2, 0, sub, 4, 15, 11 );

  TEST_RR_ZEROSRC1(34033, sub, 15, -15 );
  TEST_RR_ZEROSRC2(34034, sub, 32, 32 );
  TEST_RR_ZEROSRC12(34035, sub, 0 );
  TEST_RR_ZERODEST(34036, sub, 16, 30 );

  TEST_PASSFAIL
FUNC_EPILOGUE


# ======================================================================
# rv32ui: sw
# ======================================================================
FUNC_PROLOGUE test_rv32ui_sw

  #-------------------------------------------------------------
  # Basic tests
  #-------------------------------------------------------------

  TEST_ST_OP(35001, lw, sw, 0x0000000000aa00aa, 0,  tdat5 );
  TEST_ST_OP(35002, lw, sw, 0xffffffffaa00aa00, 4,  tdat5 );
  TEST_ST_OP(35003, lw, sw, 0x000000000aa00aa0, 8,  tdat5 );
  TEST_ST_OP(35004, lw, sw, 0xffffffffa00aa00a, 12, tdat5 );

  # Test with negative offset

  TEST_ST_OP(35005, lw, sw, 0x0000000000aa00aa, -12, tdat8 );
  TEST_ST_OP(35006, lw, sw, 0xffffffffaa00aa00, -8,  tdat8 );
  TEST_ST_OP(35007, lw, sw, 0x000000000aa00aa0, -4,  tdat8 );
  TEST_ST_OP(35008, lw, sw, 0xffffffffa00aa00a, 0,   tdat8 );

  # Test with a negative base

  TEST_CASE(35009, x5, 0x12345678, \
    la  x1, tdat9; \
    li  x2, 0x12345678; \
    addi x4, x1, -32; \
    sw x2, 32(x4); \
    lw x5, 0(x1); \
  )

  # Test with unaligned base

  TEST_CASE(35010, x5, 0x58213098, \
    la  x1, tdat9; \
    li  x2, 0x58213098; \
    addi x1, x1, -3; \
    sw x2, 7(x1); \
    la  x4, tdat10; \
    lw x5, 0(x4); \
  )

  #-------------------------------------------------------------
  # Bypassing tests
  #-------------------------------------------------------------

  TEST_ST_SRC12_BYPASS(35011, 0, 0, lw, sw, 0xffffffffaabbccdd, 0,  tdat5 );
  TEST_ST_SRC12_BYPASS(35012, 0, 1, lw, sw, 0xffffffffdaabbccd, 4,  tdat5 );
  TEST_ST_SRC12_BYPASS(35013, 0, 2, lw, sw, 0xffffffffddaabbcc, 8,  tdat5 );
  TEST_ST_SRC12_BYPASS(35014, 1, 0, lw, sw, 0xffffffffcddaabbc, 12, tdat5 );
  TEST_ST_SRC12_BYPASS(35015, 1, 1, lw, sw, 0xffffffffccddaabb, 16, tdat5 );
  TEST_ST_SRC12_BYPASS(35016, 2, 0, lw, sw, 0xffffffffbccddaab, 20, tdat5 );

  TEST_ST_SRC21_BYPASS(35017, 0, 0, lw, sw, 0x00112233, 0,  tdat5 );
  TEST_ST_SRC21_BYPASS(35018, 0, 1, lw, sw, 0x30011223, 4,  tdat5 );
  TEST_ST_SRC21_BYPASS(35019, 0, 2, lw, sw, 0x33001122, 8,  tdat5 );
  TEST_ST_SRC21_BYPASS(35020, 1, 0, lw, sw, 0x23300112, 12, tdat5 );
  TEST_ST_SRC21_BYPASS(35021, 1, 1, lw, sw, 0x22330011, 16, tdat5 );
  TEST_ST_SRC21_BYPASS(35022, 2, 0, lw, sw, 0x12233001, 20, tdat5 );

  TEST_PASSFAIL
FUNC_EPILOGUE


# ======================================================================
# rv32ui: xor
# ======================================================================
FUNC_PROLOGUE test_rv32ui_xor

  #-------------------------------------------------------------
  # Logical tests
  #-------------------------------------------------------------

  TEST_RR_OP(36001, xor, 0xf00ff00f, 0xff00ff00, 0x0f0f0f0f );
  TEST_RR_OP(36002, xor, 0xff00ff00, 0x0ff00ff0, 0xf0f0f0f0 );
  TEST_RR_OP(36003, xor, 0x0ff00ff0, 0x00ff00ff, 0x0f0f0f0f );
  TEST_RR_OP(36004, xor, 0x00ff00ff, 0xf00ff00f, 0xf0f0f0f0 );

  #-------------------------------------------------------------
  # Source/Destination tests
  #-------------------------------------------------------------

  TEST_RR_SRC1_EQ_DEST(36005, xor, 0xf00ff00f, 0xff00ff00, 0x0f0f0f0f );
  TEST_RR_SRC2_EQ_DEST(36006, xor, 0xf00ff00f, 0xff00ff00, 0x0f0f0f0f );
  TEST_RR_SRC12_EQ_DEST(36007, xor, 0x00000000, 0xff00ff00 );

  #-------------------------------------------------------------
  # Bypassing tests
  #-------------------------------------------------------------

  TEST_RR_DEST_BYPASS(36008,  0, xor, 0xf00ff00f, 0xff00ff00, 0x0f0f0f0f );
  TEST_RR_DEST_BYPASS(36009, 1, xor, 0xff00ff00, 0x0ff00ff0, 0xf0f0f0f0 );
  TEST_RR_DEST_BYPASS(36010, 2, xor, 0x0ff00ff0, 0x00ff00ff, 0x0f0f0f0f );

  TEST_RR_SRC12_BYPASS(36011, 0, 0, xor, 0xf00ff00f, 0xff00ff00, 0x0f0f0f0f );
  TEST_RR_SRC12_BYPASS(36012, 0, 1, xor, 0xff00ff00, 0x0ff00ff0, 0xf0f0f0f0 );
  TEST_RR_SRC12_BYPASS(36013, 0, 2, xor, 0x0ff00ff0, 0x00ff00ff, 0x0f0f0f0f );
  TEST_RR_SRC12_BYPASS(36014, 1, 0, xor, 0xf00ff00f, 0xff00ff00, 0x0f0f0f0f );
  TEST_RR_SRC12_BYPASS(36015, 1, 1, xor, 0xff00ff00, 0x0ff00ff0, 0xf0f0f0f0 );
  TEST_RR_SRC12_BYPASS(36016, 2, 0, xor, 0x0ff00ff0, 0x00ff00ff, 0x0f0f0f0f );

  TEST_RR_SRC21_BYPASS(36017, 0, 0, xor, 0xf00ff00f, 0xff00ff00, 0x0f0f0f0f );
  TEST_RR_SRC21_BYPASS(36018, 0, 1, xor, 0xff00ff00, 0x0ff00ff0, 0xf0f0f0f0 );
  TEST_RR_SRC21_BYPASS(36019, 0, 2, xor, 0x0ff00ff0, 0x00ff00ff, 0x0f0f0f0f );
  TEST_RR_SRC21_BYPASS(36020, 1, 0, xor, 0xf00ff00f, 0xff00ff00, 0x0f0f0f0f );
  TEST_RR_SRC21_BYPASS(36021, 1, 1, xor, 0xff00ff00, 0x0ff00ff0, 0xf0f0f0f0 );
  TEST_RR_SRC21_BYPASS(36022, 2, 0, xor, 0x0ff00ff0, 0x00ff00ff, 0x0f0f0f0f );

  TEST_RR_ZEROSRC1(36023, xor, 0xff00ff00, 0xff00ff00 );
  TEST_RR_ZEROSRC2(36024, xor, 0x00ff00ff, 0x00ff00ff );
  TEST_RR_ZEROSRC12(36025, xor, 0 );
  TEST_RR_ZERODEST(36026, xor, 0x11111111, 0x22222222 );

  TEST_PASSFAIL
FUNC_EPILOGUE


# ======================================================================
# rv32ui: xori
# ======================================================================
FUNC_PROLOGUE test_rv32ui_xori

  #-------------------------------------------------------------
  # Logical tests
  #-------------------------------------------------------------

  TEST_IMM_OP(37001, xori, 0xffffffffff00f00f, 0x0000000000ff0f00, 0xf0f );
  TEST_IMM_OP(37002, xori, 0x000000000ff00f00, 0x000000000ff00ff0, 0x0f0 );
  TEST_IMM_OP(37003, xori, 0x0000000000ff0ff0, 0x0000000000ff08ff, 0x70f );
  TEST_IMM_OP(37004, xori, 0xfffffffff00ff0ff, 0xfffffffff00ff00f, 0x0f0 );

  #-------------------------------------------------------------
  # Source/Destination tests
  #-------------------------------------------------------------

  TEST_IMM_SRC1_EQ_DEST(37005, xori, 0xffffffffff00f00f, 0xffffffffff00f700, 0x70f );

   #-------------------------------------------------------------
  # Bypassing tests
  #-------------------------------------------------------------

  TEST_IMM_DEST_BYPASS(37006,  0, xori, 0x000000000ff00f00, 0x000000000ff00ff0, 0x0f0 );
  TEST_IMM_DEST_BYPASS(37007,  1, xori, 0x0000000000ff0ff0, 0x0000000000ff08ff, 0x70f );
  TEST_IMM_DEST_BYPASS(37008,  2, xori, 0xfffffffff00ff0ff, 0xfffffffff00ff00f, 0x0f0 );

  TEST_IMM_SRC1_BYPASS(37009, 0, xori, 0x000000000ff00f00, 0x000000000ff00ff0, 0x0f0 );
  TEST_IMM_SRC1_BYPASS(37010, 1, xori, 0x0000000000ff0ff0, 0x0000000000ff0fff, 0x00f );
  TEST_IMM_SRC1_BYPASS(37011, 2, xori, 0xfffffffff00ff0ff, 0xfffffffff00ff00f, 0x0f0 );

  TEST_IMM_ZEROSRC1(37012, xori, 0x0f0, 0x0f0 );
  TEST_IMM_ZERODEST(37013, xori, 0x00ff00ff, 0x70f );

  TEST_PASSFAIL
FUNC_EPILOGUE

# ======================================================================
# rv32ui: simple
# ======================================================================
FUNC_PROLOGUE test_rv32ui_simple
  li TESTNUM, 1
  li a0, 0
  TEST_PASSFAIL
FUNC_EPILOGUE


# ======================================================================
# rv32ui: ld_st
# ======================================================================
FUNC_PROLOGUE test_rv32ui_ld_st

  #-------------------------------------------------------------
  # Bypassing Tests
  #-------------------------------------------------------------

  # Test sb and lb (signed byte)
  TEST_LD_ST_BYPASS(39001,  lb,  sb, 0xffffffffffffffdd, 0, tdat_ldst );
  TEST_LD_ST_BYPASS(39002,  lb,  sb, 0xffffffffffffffcd, 1, tdat_ldst );
  TEST_LD_ST_BYPASS(39003,  lb,  sb, 0xffffffffffffffcc, 2, tdat_ldst );
  TEST_LD_ST_BYPASS(39004,  lb,  sb, 0xffffffffffffffbc, 3, tdat_ldst );
  TEST_LD_ST_BYPASS(39005,  lb,  sb, 0xffffffffffffffbb, 4, tdat_ldst );
  TEST_LD_ST_BYPASS(39006,  lb,  sb, 0xffffffffffffffab, 5, tdat_ldst );

  TEST_LD_ST_BYPASS(39007,  lb, sb, 0x33, 0, tdat_ldst );
  TEST_LD_ST_BYPASS(39008,  lb, sb, 0x23, 1, tdat_ldst );
  TEST_LD_ST_BYPASS(39009, lb, sb, 0x22, 2, tdat_ldst );
  TEST_LD_ST_BYPASS(39010, lb, sb, 0x12, 3, tdat_ldst );
  TEST_LD_ST_BYPASS(39011, lb, sb, 0x11, 4, tdat_ldst );
  TEST_LD_ST_BYPASS(39012, lb, sb, 0x01, 5, tdat_ldst );

  # Test sb and lbu (unsigned byte)
  TEST_LD_ST_BYPASS(39013, lbu, sb, 0x33, 0, tdat_ldst );
  TEST_LD_ST_BYPASS(39014, lbu, sb, 0x23, 1, tdat_ldst );
  TEST_LD_ST_BYPASS(39015, lbu, sb, 0x22, 2, tdat_ldst );
  TEST_LD_ST_BYPASS(39016, lbu, sb, 0x12, 3, tdat_ldst );
  TEST_LD_ST_BYPASS(39017, lbu, sb, 0x11, 4, tdat_ldst );
  TEST_LD_ST_BYPASS(39018, lbu, sb, 0x01, 5, tdat_ldst );

  # Test sw and lw (signed word)
  TEST_LD_ST_BYPASS(39019, lw, sw, 0xffffffffaabbccdd, 0,  tdat_ldst );
  TEST_LD_ST_BYPASS(39020, lw, sw, 0xffffffffdaabbccd, 4,  tdat_ldst );
  TEST_LD_ST_BYPASS(39021, lw, sw, 0xffffffffddaabbcc, 8,  tdat_ldst );
  TEST_LD_ST_BYPASS(39022, lw, sw, 0xffffffffcddaabbc, 12, tdat_ldst );
  TEST_LD_ST_BYPASS(39023, lw, sw, 0xffffffffccddaabb, 16, tdat_ldst );
  TEST_LD_ST_BYPASS(39024, lw, sw, 0xffffffffbccddaab, 20, tdat_ldst );

  TEST_LD_ST_BYPASS(39025, lw, sw, 0x00112233, 0,  tdat_ldst );
  TEST_LD_ST_BYPASS(39026, lw, sw, 0x30011223, 4,  tdat_ldst );
  TEST_LD_ST_BYPASS(39027, lw, sw, 0x33001122, 8,  tdat_ldst );
  TEST_LD_ST_BYPASS(39028, lw, sw, 0x23300112, 12, tdat_ldst );
  TEST_LD_ST_BYPASS(39029, lw, sw, 0x22330011, 16, tdat_ldst );
  TEST_LD_ST_BYPASS(39030, lw, sw, 0x12233001, 20, tdat_ldst );

  # Test sh and lh (signed halfword)
  TEST_LD_ST_BYPASS(39031, lh, sh, 0xffffffffffffccdd, 0, tdat_ldst );
  TEST_LD_ST_BYPASS(39032, lh, sh, 0xffffffffffffbccd, 2, tdat_ldst );
  TEST_LD_ST_BYPASS(39033, lh, sh, 0xffffffffffffbbcc, 4, tdat_ldst );
  TEST_LD_ST_BYPASS(39034, lh, sh, 0xffffffffffffabbc, 6, tdat_ldst );
  TEST_LD_ST_BYPASS(39035, lh, sh, 0xffffffffffffaabb, 8, tdat_ldst );
  TEST_LD_ST_BYPASS(39036, lh, sh, 0xffffffffffffdaab, 10, tdat_ldst );

  TEST_LD_ST_BYPASS(39037, lh, sh, 0x2233, 0, tdat_ldst );
  TEST_LD_ST_BYPASS(39038, lh, sh, 0x1223, 2, tdat_ldst );
  TEST_LD_ST_BYPASS(39039, lh, sh, 0x1122, 4, tdat_ldst );
  TEST_LD_ST_BYPASS(39040, lh, sh, 0x0112, 6, tdat_ldst );
  TEST_LD_ST_BYPASS(39041, lh, sh, 0x0011, 8, tdat_ldst );
  TEST_LD_ST_BYPASS(39042, lh, sh, 0x3001, 10, tdat_ldst );

  # Test sh and lhu (unsigned halfword)
  TEST_LD_ST_BYPASS(39043, lhu, sh, 0x2233, 0, tdat_ldst );
  TEST_LD_ST_BYPASS(39044, lhu, sh, 0x1223, 2, tdat_ldst );
  TEST_LD_ST_BYPASS(39045, lhu, sh, 0x1122, 4, tdat_ldst );
  TEST_LD_ST_BYPASS(39046, lhu, sh, 0x0112, 6, tdat_ldst );
  TEST_LD_ST_BYPASS(39047, lhu, sh, 0x0011, 8, tdat_ldst );
  TEST_LD_ST_BYPASS(39048, lhu, sh, 0x3001, 10, tdat_ldst );

  # RV64-specific tests for ld, sd, and lwu
#if __riscv_xlen == 64
  # Test sd and ld (doubleword)
  TEST_LD_ST_BYPASS(39049, ld, sd, 0x0011223344556677, 0,  tdat_ldst );
  TEST_LD_ST_BYPASS(39050, ld, sd, 0x1122334455667788, 8,  tdat_ldst );
  TEST_LD_ST_BYPASS(39051, ld, sd, 0x2233445566778899, 16, tdat_ldst );
  TEST_LD_ST_BYPASS(39052, ld, sd, 0xabbccdd, 0,  tdat_ldst );
  TEST_LD_ST_BYPASS(39053, ld, sd, 0xaabbccd, 8,  tdat_ldst );
  TEST_LD_ST_BYPASS(39054, ld, sd, 0xdaabbcc, 16, tdat_ldst );
  TEST_LD_ST_BYPASS(39055, ld, sd, 0xddaabbc, 24, tdat_ldst );
  TEST_LD_ST_BYPASS(39056, ld, sd, 0xcddaabb, 32, tdat_ldst );
  TEST_LD_ST_BYPASS(39057, ld, sd, 0xccddaab, 40, tdat_ldst );

  TEST_LD_ST_BYPASS(39058, ld, sd, 0x00112233, 0,  tdat_ldst );
  TEST_LD_ST_BYPASS(39059, ld, sd, 0x30011223, 8,  tdat_ldst );
  TEST_LD_ST_BYPASS(39060, ld, sd, 0x33001122, 16, tdat_ldst );
  TEST_LD_ST_BYPASS(39061, ld, sd, 0x23300112, 24, tdat_ldst );
  TEST_LD_ST_BYPASS(39062, ld, sd, 0x22330011, 32, tdat_ldst );
  TEST_LD_ST_BYPASS(39063, ld, sd, 0x12233001, 40, tdat_ldst );

  # Test sw and lwu (unsigned word)
  TEST_LD_ST_BYPASS(39064, lwu, sw, 0x00112233, 0,  tdat_ldst );
  TEST_LD_ST_BYPASS(39065, lwu, sw, 0x33001122, 8,  tdat_ldst );
  TEST_LD_ST_BYPASS(39066, lwu, sw, 0x30011223, 4,  tdat_ldst );
  TEST_LD_ST_BYPASS(39067, lwu, sw, 0x23300112, 12, tdat_ldst );
  TEST_LD_ST_BYPASS(39068, lwu, sw, 0x22330011, 16, tdat_ldst );
  TEST_LD_ST_BYPASS(39069, lwu, sw, 0x12233001, 20, tdat_ldst );
#endif

  li a0, 0xef         # Immediate load for manual store test
  la a1, tdat_ldst         # Load address of tdat_ldst
  sb a0, 3(a1)        # Store byte at offset 3 of tdat_ldst
  lb a2, 3(a1)        # Load byte back for verification

  TEST_PASSFAIL
FUNC_EPILOGUE


# ======================================================================
# rv32ui: st_ld
# ======================================================================
FUNC_PROLOGUE test_rv32ui_st_ld

  #-------------------------------------------------------------
  # Bypassing Tests
  #-------------------------------------------------------------

  # Test sb and lb (signed byte)
  TEST_ST_LD_BYPASS(40001,  lb,  sb, 0xffffffffffffffdd, 0, tdat_stld );
  TEST_ST_LD_BYPASS(40002,  lb,  sb, 0xffffffffffffffcd, 1, tdat_stld );
  TEST_ST_LD_BYPASS(40003,  lb,  sb, 0xffffffffffffffcc, 2, tdat_stld );
  TEST_ST_LD_BYPASS(40004,  lb,  sb, 0xffffffffffffffbc, 3, tdat_stld );
  TEST_ST_LD_BYPASS(40005,  lb,  sb, 0xffffffffffffffbb, 4, tdat_stld );
  TEST_ST_LD_BYPASS(40006,  lb,  sb, 0xffffffffffffffab, 5, tdat_stld );

  TEST_ST_LD_BYPASS(40007,  lb, sb, 0x33, 0, tdat_stld );
  TEST_ST_LD_BYPASS(40008,  lb, sb, 0x23, 1, tdat_stld );
  TEST_ST_LD_BYPASS(40009, lb, sb, 0x22, 2, tdat_stld );
  TEST_ST_LD_BYPASS(40010, lb, sb, 0x12, 3, tdat_stld );
  TEST_ST_LD_BYPASS(40011, lb, sb, 0x11, 4, tdat_stld );
  TEST_ST_LD_BYPASS(40012, lb, sb, 0x01, 5, tdat_stld );

  # Test sb and lbu (unsigned byte)
  TEST_ST_LD_BYPASS(40013, lbu, sb, 0x33, 0, tdat_stld );
  TEST_ST_LD_BYPASS(40014, lbu, sb, 0x23, 1, tdat_stld );
  TEST_ST_LD_BYPASS(40015, lbu, sb, 0x22, 2, tdat_stld );
  TEST_ST_LD_BYPASS(40016, lbu, sb, 0x12, 3, tdat_stld );
  TEST_ST_LD_BYPASS(40017, lbu, sb, 0x11, 4, tdat_stld );
  TEST_ST_LD_BYPASS(40018, lbu, sb, 0x01, 5, tdat_stld );

  # Test sw and lw (signed word)
  TEST_ST_LD_BYPASS(40019, lw, sw, 0xffffffffaabbccdd, 0,  tdat_stld );
  TEST_ST_LD_BYPASS(40020, lw, sw, 0xffffffffdaabbccd, 4,  tdat_stld );
  TEST_ST_LD_BYPASS(40021, lw, sw, 0xffffffffddaabbcc, 8,  tdat_stld );
  TEST_ST_LD_BYPASS(40022, lw, sw, 0xffffffffcddaabbc, 12, tdat_stld );
  TEST_ST_LD_BYPASS(40023, lw, sw, 0xffffffffccddaabb, 16, tdat_stld );
  TEST_ST_LD_BYPASS(40024, lw, sw, 0xffffffffbccddaab, 20, tdat_stld );

  TEST_ST_LD_BYPASS(40025, lw, sw, 0x00112233, 0,  tdat_stld );
  TEST_ST_LD_BYPASS(40026, lw, sw, 0x30011223, 4,  tdat_stld );
  TEST_ST_LD_BYPASS(40027, lw, sw, 0x33001122, 8,  tdat_stld );
  TEST_ST_LD_BYPASS(40028, lw, sw, 0x23300112, 12, tdat_stld );
  TEST_ST_LD_BYPASS(40029, lw, sw, 0x22330011, 16, tdat_stld );
  TEST_ST_LD_BYPASS(40030, lw, sw, 0x12233001, 20, tdat_stld );

  # Test sh and lh (signed halfword)
  TEST_ST_LD_BYPASS(40031, lh, sh, 0xffffffffffffccdd, 0, tdat_stld );
  TEST_ST_LD_BYPASS(40032, lh, sh, 0xffffffffffffbccd, 2, tdat_stld );
  TEST_ST_LD_BYPASS(40033, lh, sh, 0xffffffffffffbbcc, 4, tdat_stld );
  TEST_ST_LD_BYPASS(40034, lh, sh, 0xffffffffffffabbc, 6, tdat_stld );
  TEST_ST_LD_BYPASS(40035, lh, sh, 0xffffffffffffaabb, 8, tdat_stld );
  TEST_ST_LD_BYPASS(40036, lh, sh, 0xffffffffffffdaab, 10, tdat_stld );

  TEST_ST_LD_BYPASS(40037, lh, sh, 0x2233, 0, tdat_stld );
  TEST_ST_LD_BYPASS(40038, lh, sh, 0x1223, 2, tdat_stld );
  TEST_ST_LD_BYPASS(40039, lh, sh, 0x1122, 4, tdat_stld );
  TEST_ST_LD_BYPASS(40040, lh, sh, 0x0112, 6, tdat_stld );
  TEST_ST_LD_BYPASS(40041, lh, sh, 0x0011, 8, tdat_stld );
  TEST_ST_LD_BYPASS(40042, lh, sh, 0x3001, 10, tdat_stld );

  # Test sh and lhu (unsigned halfword)
  TEST_ST_LD_BYPASS(40043, lhu, sh, 0x2233, 0, tdat_stld );
  TEST_ST_LD_BYPASS(40044, lhu, sh, 0x1223, 2, tdat_stld );
  TEST_ST_LD_BYPASS(40045, lhu, sh, 0x1122, 4, tdat_stld );
  TEST_ST_LD_BYPASS(40046, lhu, sh, 0x0112, 6, tdat_stld );
  TEST_ST_LD_BYPASS(40047, lhu, sh, 0x0011, 8, tdat_stld );
  TEST_ST_LD_BYPASS(40048, lhu, sh, 0x3001, 10, tdat_stld );

  # RV64-specific tests for ld, sd, and lwu
#if __riscv_xlen == 64
  # Test sd and ld (doubleword)
  TEST_ST_LD_BYPASS(40049, ld, sd, 0x0011223344556677, 0,  tdat_stld );
  TEST_ST_LD_BYPASS(40050, ld, sd, 0x1122334455667788, 8,  tdat_stld );
  TEST_ST_LD_BYPASS(40051, ld, sd, 0x2233445566778899, 16, tdat_stld );
  TEST_ST_LD_BYPASS(40052, ld, sd, 0xabbccdd, 0,  tdat_stld );
  TEST_ST_LD_BYPASS(40053, ld, sd, 0xaabbccd, 8,  tdat_stld );
  TEST_ST_LD_BYPASS(40054, ld, sd, 0xdaabbcc, 16, tdat_stld );
  TEST_ST_LD_BYPASS(40055, ld, sd, 0xddaabbc, 24, tdat_stld );
  TEST_ST_LD_BYPASS(40056, ld, sd, 0xcddaabb, 32, tdat_stld );
  TEST_ST_LD_BYPASS(40057, ld, sd, 0xccddaab, 40, tdat_stld );

  TEST_ST_LD_BYPASS(40058, ld, sd, 0x00112233, 0,  tdat_stld );
  TEST_ST_LD_BYPASS(40059, ld, sd, 0x30011223, 8,  tdat_stld );
  TEST_ST_LD_BYPASS(40060, ld, sd, 0x33001122, 16, tdat_stld );
  TEST_ST_LD_BYPASS(40061, ld, sd, 0x23300112, 24, tdat_stld );
  TEST_ST_LD_BYPASS(40062, ld, sd, 0x22330011, 32, tdat_stld );
  TEST_ST_LD_BYPASS(40063, ld, sd, 0x12233001, 40, tdat_stld );

  # Test sw and lwu (unsigned word)
  TEST_ST_LD_BYPASS(40064, lwu, sw, 0x00112233, 0,  tdat_stld );
  TEST_ST_LD_BYPASS(40065, lwu, sw, 0x33001122, 8,  tdat_stld );
  TEST_ST_LD_BYPASS(40066, lwu, sw, 0x30011223, 4,  tdat_stld );
  TEST_ST_LD_BYPASS(40067, lwu, sw, 0x23300112, 12, tdat_stld );
  TEST_ST_LD_BYPASS(40068, lwu, sw, 0x22330011, 16, tdat_stld );
  TEST_ST_LD_BYPASS(40069, lwu, sw, 0x12233001, 20, tdat_stld );
#endif

  li a0, 0xef         # Immediate load for manual store test
  la a1, tdat_stld         # Load address of tdat_stld
  sb a0, 3(a1)        # Store byte at offset 3 of tdat_stld
  lb a2, 3(a1)        # Load byte back for verification

  TEST_PASSFAIL
FUNC_EPILOGUE


# ======================================================================
# rv32ui: fence_i
# ======================================================================
FUNC_PROLOGUE test_rv32ui_fence_i

li a3, 111
lh a0, insn_fence_i
lh a1, insn_fence_i+2

# test I$ hit
.align 6
sh a0, 2f, t0
sh a1, 2f+2, t0
fence.i

la a5, 2f
jalr t1, a5, 0
TEST_CASE(41001, a3, 444, nop )

# test prefetcher hit
li a4, 100
1: addi a4, a4, -1
bnez a4, 1b

sh a0, 3f, t0
sh a1, 3f+2, t0
fence.i

.align 6
la a5, 3f
jalr t1, a5, 0
TEST_CASE(41002, a3, 777, nop )

  TEST_PASSFAIL
FUNC_EPILOGUE


# ======================================================================
# rv32ui: ma_data
# ======================================================================
FUNC_PROLOGUE test_rv32ui_ma_data

  la s0, data_ma

#define SEXT(x, n) ((-((x) >> ((n)-1)) << (n)) | ((x) & ((1 << (n))-1)))

/* Check that a misaligned load reads the correct value. */
#define MISALIGNED_LOAD_TEST(testnum, insn, base, offset, res) \
  li TESTNUM, testnum; \
  li t1, res; \
  insn t2, offset(base); \
  bne t1, t2, fail; \
1:

# within quadword
  MISALIGNED_LOAD_TEST(1,  lh,  s0, 1, SEXT(0x0201, 16))
  MISALIGNED_LOAD_TEST(2,  lhu, s0, 1, 0x0201)
  MISALIGNED_LOAD_TEST(3,  lw,  s0, 1, SEXT(0x04030201, 32))
  MISALIGNED_LOAD_TEST(4,  lw,  s0, 2, SEXT(0x05040302, 32))
  MISALIGNED_LOAD_TEST(5,  lw,  s0, 3, SEXT(0x06050403, 32))

#if __riscv_xlen == 64
  MISALIGNED_LOAD_TEST(6,  lwu, s0, 1, 0x04030201)
  MISALIGNED_LOAD_TEST(7,  lwu, s0, 2, 0x05040302)
  MISALIGNED_LOAD_TEST(8,  lwu, s0, 3, 0x06050403)

  MISALIGNED_LOAD_TEST(9,  ld, s0, 1, 0x0807060504030201)
  MISALIGNED_LOAD_TEST(10, ld, s0, 2, 0x0908070605040302)
  MISALIGNED_LOAD_TEST(11, ld, s0, 3, 0x0a09080706050403)
  MISALIGNED_LOAD_TEST(12, ld, s0, 4, 0x0b0a090807060504)
  MISALIGNED_LOAD_TEST(13, ld, s0, 5, 0x0c0b0a0908070605)
  MISALIGNED_LOAD_TEST(14, ld, s0, 6, 0x0d0c0b0a09080706)
  MISALIGNED_LOAD_TEST(15, ld, s0, 7, 0x0e0d0c0b0a090807)
#endif

# octword crossing
  MISALIGNED_LOAD_TEST(16, lh,  s0, 31, SEXT(0x201f, 16))
  MISALIGNED_LOAD_TEST(17, lhu, s0, 31, 0x201f)
  MISALIGNED_LOAD_TEST(18, lw,  s0, 29, SEXT(0x201f1e1d, 32))
  MISALIGNED_LOAD_TEST(19, lw,  s0, 30, SEXT(0x21201f1e, 32))
  MISALIGNED_LOAD_TEST(20, lw,  s0, 31, SEXT(0x2221201f, 32))

#if __riscv_xlen == 64
  MISALIGNED_LOAD_TEST(21, lwu, s0, 29, 0x201f1e1d)
  MISALIGNED_LOAD_TEST(22, lwu, s0, 30, 0x21201f1e)
  MISALIGNED_LOAD_TEST(23, lwu, s0, 31, 0x2221201f)

  MISALIGNED_LOAD_TEST(24, ld, s0, 25, 0x201f1e1d1c1b1a19)
  MISALIGNED_LOAD_TEST(25, ld, s0, 26, 0x21201f1e1d1c1b1a)
  MISALIGNED_LOAD_TEST(26, ld, s0, 27, 0x2221201f1e1d1c1b)
  MISALIGNED_LOAD_TEST(27, ld, s0, 28, 0x232221201f1e1d1c)
  MISALIGNED_LOAD_TEST(28, ld, s0, 29, 0x24232221201f1e1d)
  MISALIGNED_LOAD_TEST(29, ld, s0, 30, 0x2524232221201f1e)
  MISALIGNED_LOAD_TEST(30, ld, s0, 31, 0x262524232221201f)
#endif

# cacheline crossing
  MISALIGNED_LOAD_TEST(31, lh,  s0, 63, SEXT(0x403f, 16))
  MISALIGNED_LOAD_TEST(32, lhu, s0, 63, 0x403f)
  MISALIGNED_LOAD_TEST(33, lw,  s0, 61, SEXT(0x403f3e3d, 32))
  MISALIGNED_LOAD_TEST(34, lw,  s0, 62, SEXT(0x41403f3e, 32))
  MISALIGNED_LOAD_TEST(35, lw,  s0, 63, SEXT(0x4241403f, 32))

#if __riscv_xlen == 64
  MISALIGNED_LOAD_TEST(36, lwu, s0, 61, 0x403f3e3d)
  MISALIGNED_LOAD_TEST(37, lwu, s0, 62, 0x41403f3e)
  MISALIGNED_LOAD_TEST(38, lwu, s0, 63, 0x4241403f)

  MISALIGNED_LOAD_TEST(39, ld, s0, 57, 0x403f3e3d3c3b3a39)
  MISALIGNED_LOAD_TEST(40, ld, s0, 58, 0x41403f3e3d3c3b3a)
  MISALIGNED_LOAD_TEST(41, ld, s0, 59, 0x4241403f3e3d3c3b)
  MISALIGNED_LOAD_TEST(42, ld, s0, 60, 0x434241403f3e3d3c)
  MISALIGNED_LOAD_TEST(43, ld, s0, 61, 0x44434241403f3e3d)
  MISALIGNED_LOAD_TEST(44, ld, s0, 62, 0x4544434241403f3e)
  MISALIGNED_LOAD_TEST(45, ld, s0, 63, 0x464544434241403f)
#endif


/* Check that a misaligned store writes the correct value. */
#define MISALIGNED_STORE_TEST(testnum, st_insn, ld_insn, base, offset, st_data) \
  li TESTNUM, testnum; \
  li t1, st_data; \
  st_insn t1, offset(base); \
  ld_insn t2, offset(base); \
  bne t1, t2, fail; \
1:

# within quadword
  MISALIGNED_STORE_TEST(46, sh, lh,  s0, 1, SEXT(0x8180, 16))
  MISALIGNED_STORE_TEST(47, sh, lhu, s0, 1, 0x8382)
  MISALIGNED_STORE_TEST(48, sw, lw,  s0, 1, SEXT(0x87868584, 32))
  MISALIGNED_STORE_TEST(49, sw, lw,  s0, 2, SEXT(0x8b8a8988, 32))
  MISALIGNED_STORE_TEST(50, sw, lw,  s0, 3, SEXT(0x8f8e8d8c, 32))

#if __riscv_xlen == 64
  MISALIGNED_STORE_TEST(51, sw, lwu, s0, 1, 0x93929190)
  MISALIGNED_STORE_TEST(52, sw, lwu, s0, 2, 0x97969594)
  MISALIGNED_STORE_TEST(53, sw, lwu, s0, 3, 0x9b9a9998)

  MISALIGNED_STORE_TEST(54, sd, ld, s0, 1, 0xa3a2a1a09f9e9d9c)
  MISALIGNED_STORE_TEST(55, sd, ld, s0, 2, 0xabaaa9a8a7a6a5a4)
  MISALIGNED_STORE_TEST(56, sd, ld, s0, 3, 0xb3b2b1b0afaeadac)
  MISALIGNED_STORE_TEST(57, sd, ld, s0, 4, 0xbbbab9b8b7b6b5b4)
  MISALIGNED_STORE_TEST(58, sd, ld, s0, 5, 0xc3c2c1c0bfbebdbc)
  MISALIGNED_STORE_TEST(59, sd, ld, s0, 6, 0xcbcac9c8c7c6c5c4)
  MISALIGNED_STORE_TEST(60, sd, ld, s0, 7, 0xd3d2d1d0cfcecdcc)
#endif

# octword crossing
  MISALIGNED_STORE_TEST(61, sh, lh,  s0, 31, SEXT(0xd5d4, 16))
  MISALIGNED_STORE_TEST(62, sh, lhu, s0, 31, 0xd7d6)
  MISALIGNED_STORE_TEST(63, sw, lw,  s0, 29, SEXT(0xdbdad9d8, 32))
  MISALIGNED_STORE_TEST(64, sw, lw,  s0, 30, SEXT(0xdfdedddc, 32))
  MISALIGNED_STORE_TEST(65, sw, lw,  s0, 31, SEXT(0xe3e2e1e0, 32))

#if __riscv_xlen == 64
  MISALIGNED_STORE_TEST(66, sw, lwu, s0, 29, 0xe7e6e5e4)
  MISALIGNED_STORE_TEST(67, sw, lwu, s0, 30, 0xebeae9e8)
  MISALIGNED_STORE_TEST(68, sw, lwu, s0, 31, 0xefeeedec)

  MISALIGNED_STORE_TEST(69, sd, ld, s0, 25, 0xf7f6f5f4f3f2f1f0)
  MISALIGNED_STORE_TEST(70, sd, ld, s0, 26, 0xfffefdfcfbfaf9f8)
  MISALIGNED_STORE_TEST(71, sd, ld, s0, 27, 0x0706050403020100)
  MISALIGNED_STORE_TEST(72, sd, ld, s0, 28, 0x0f0e0d0c0b0a0908)
  MISALIGNED_STORE_TEST(73, sd, ld, s0, 29, 0x1716151413121110)
  MISALIGNED_STORE_TEST(74, sd, ld, s0, 30, 0x1f1e1d1c1b1a1918)
  MISALIGNED_STORE_TEST(75, sd, ld, s0, 31, 0x2726252423222120)
#endif

# cacheline crossing
  MISALIGNED_STORE_TEST(76, sh, lh,  s0, 63, SEXT(0x3534, 16))
  MISALIGNED_STORE_TEST(77, sh, lhu, s0, 63, 0x3736)
  MISALIGNED_STORE_TEST(78, sw, lw,  s0, 61, SEXT(0x3b3a3938, 32))
  MISALIGNED_STORE_TEST(79, sw, lw,  s0, 62, SEXT(0x3f3e3d3c, 32))
  MISALIGNED_STORE_TEST(80, sw, lw,  s0, 63, SEXT(0x43424140, 32))

#if __riscv_xlen == 64
  MISALIGNED_STORE_TEST(81, sw, lwu, s0, 61, 0x47464544)
  MISALIGNED_STORE_TEST(82, sw, lwu, s0, 62, 0x4b4a4948)
  MISALIGNED_STORE_TEST(83, sw, lwu, s0, 63, 0x4f4e4d4c)

  MISALIGNED_STORE_TEST(84, sd, ld, s0, 57, 0x5756555453525150)
  MISALIGNED_STORE_TEST(85, sd, ld, s0, 58, 0x5f5e5d5c5b5a5958)
  MISALIGNED_STORE_TEST(86, sd, ld, s0, 59, 0x6766656463626160)
  MISALIGNED_STORE_TEST(87, sd, ld, s0, 60, 0x6f6e6d6c6b6a6968)
  MISALIGNED_STORE_TEST(88, sd, ld, s0, 61, 0x7776757473727170)
  MISALIGNED_STORE_TEST(89, sd, ld, s0, 62, 0x7f7e7d7c7b7a7978)
  MISALIGNED_STORE_TEST(90, sd, ld, s0, 63, 0x8786858483828180)
#endif


/* Check that a misaligned store writes the correct value, checked by a narrower load. */
#define MISMATCHED_STORE_TEST(testnum, st_insn, ld_insn, base, st_offset, ld_offset, st_data, ld_data) \
  li TESTNUM, testnum; \
  li t1, st_data; \
  li t2, ld_data; \
  st_insn t1, st_offset(base); \
  ld_insn t3, ld_offset(base); \
  bne t2, t3, fail; \
1:

# within quadword
  MISMATCHED_STORE_TEST(91,  sh, lb,  s0, 1, 1, 0x9998, SEXT(0x98, 8))
  MISMATCHED_STORE_TEST(92,  sh, lb,  s0, 1, 2, 0x9b9a, SEXT(0x9b, 8))
  MISMATCHED_STORE_TEST(93,  sh, lbu, s0, 1, 1, 0x9d9c, 0x9c)
  MISMATCHED_STORE_TEST(94,  sh, lbu, s0, 1, 2, 0x9f9e, 0x9f)
  MISMATCHED_STORE_TEST(95,  sw, lb,  s0, 1, 1, 0xa3a2a1a0, SEXT(0xa0, 8))
  MISMATCHED_STORE_TEST(96,  sw, lbu, s0, 2, 3, 0xa7a6a5a4, 0xa5)
  MISMATCHED_STORE_TEST(97,  sw, lh,  s0, 3, 4, 0xabaaa9a8, SEXT(0xaaa9, 16))
  MISMATCHED_STORE_TEST(98,  sw, lhu, s0, 3, 5, 0xafaeadac, 0xafae)

#if __riscv_xlen == 64
  MISMATCHED_STORE_TEST(99,  sd, lb,  s0, 1, 7, 0xb7b6b5b4b3b2b1b0, SEXT(0xb6, 8))
  MISMATCHED_STORE_TEST(100, sd, lbu, s0, 2, 3, 0xbfbebdbcbbbab9b8, 0xb9)
  MISMATCHED_STORE_TEST(101, sd, lh,  s0, 3, 9, 0xc7c6c5c4c3c2c1c0, SEXT(0xc7c6, 16))
  MISMATCHED_STORE_TEST(102, sd, lhu, s0, 4, 5, 0xcfcecdcccbcac9c8, 0xcac9)
  MISMATCHED_STORE_TEST(103, sd, lw,  s0, 5, 9, 0xd7d6d5d4d3d2d1d0, SEXT(0xd7d6d5d4, 32))
  MISMATCHED_STORE_TEST(104, sd, lw,  s0, 6, 8, 0xdfdedddcdbdad9d8, SEXT(0xdddcdbda, 32))
  MISMATCHED_STORE_TEST(105, sd, lwu, s0, 7, 8, 0xe7e6e5e4e3e2e1e0, 0xe4e3e2e1)
#endif

# octword crossing
  MISMATCHED_STORE_TEST(106, sh, lb,  s0, 31, 31, 0xe9e8, SEXT(0xe8, 8))
  MISMATCHED_STORE_TEST(107, sh, lb,  s0, 31, 32, 0xebea, SEXT(0xeb, 8))
  MISMATCHED_STORE_TEST(108, sh, lbu, s0, 31, 31, 0xedec, 0xec)
  MISMATCHED_STORE_TEST(109, sh, lbu, s0, 31, 32, 0xefee, 0xef)
  MISMATCHED_STORE_TEST(110, sw, lb,  s0, 29, 29, 0xf3f2f1f0, SEXT(0xf0, 8))
  MISMATCHED_STORE_TEST(111, sw, lbu, s0, 30, 32, 0xf7f6f5f4, 0xf6)
  MISMATCHED_STORE_TEST(112, sw, lh,  s0, 29, 31, 0xfbfaf9f8, SEXT(0xfbfa, 16))
  MISMATCHED_STORE_TEST(113, sw, lhu, s0, 31, 31, 0xfffefdfc, 0xfdfc)

#if __riscv_xlen == 64
  MISMATCHED_STORE_TEST(114, sd, lb,  s0, 25, 32, 0x0706050403020100, SEXT(0x07, 8))
  MISMATCHED_STORE_TEST(115, sd, lbu, s0, 26, 33, 0x0f0e0d0c0b0a0908, 0x0f)
  MISMATCHED_STORE_TEST(116, sd, lh,  s0, 27, 31, 0x1716151413121110, SEXT(0x1514, 16))
  MISMATCHED_STORE_TEST(117, sd, lhu, s0, 28, 31, 0x1f1e1d1c1b1a1918, 0x1c1b)
  MISMATCHED_STORE_TEST(118, sd, lw,  s0, 29, 29, 0x2726252423222120, SEXT(0x23222120, 32))
  MISMATCHED_STORE_TEST(119, sd, lw,  s0, 30, 30, 0x2f2e2d2c2b2a2928, SEXT(0x2b2a2928, 32))
  MISMATCHED_STORE_TEST(120, sd, lwu, s0, 31, 31, 0x3736353433323130, 0x33323130)
#endif

# cacheline crossing
  MISMATCHED_STORE_TEST(121, sh, lb,  s0, 63, 63, 0x4948, SEXT(0x48, 8))
  MISMATCHED_STORE_TEST(122, sh, lb,  s0, 63, 64, 0x4b4a, SEXT(0x4b, 8))
  MISMATCHED_STORE_TEST(123, sh, lbu, s0, 63, 63, 0x4d4c, 0x4c)
  MISMATCHED_STORE_TEST(124, sh, lbu, s0, 63, 64, 0x4f4e, 0x4f)
  MISMATCHED_STORE_TEST(125, sw, lb,  s0, 61, 61, 0x53525150, SEXT(0x50, 8))
  MISMATCHED_STORE_TEST(126, sw, lbu, s0, 62, 64, 0x57565554, 0x56)
  MISMATCHED_STORE_TEST(127, sw, lh,  s0, 61, 63, 0x5b5a5958, SEXT(0x5b5a, 16))
  MISMATCHED_STORE_TEST(128, sw, lhu, s0, 63, 63, 0x5f5e5d5c, 0x5d5c)

#if __riscv_xlen == 64
  MISMATCHED_STORE_TEST(129, sd, lb,  s0, 57, 64, 0x6766656463626160, SEXT(0x67, 8))
  MISMATCHED_STORE_TEST(130, sd, lbu, s0, 58, 65, 0x6f6e6d6c6b6a6968, 0x6f)
  MISMATCHED_STORE_TEST(131, sd, lh,  s0, 59, 63, 0x7776757473727170, SEXT(0x7574, 16))
  MISMATCHED_STORE_TEST(132, sd, lhu, s0, 60, 63, 0x7f7e7d7c7b7a7978, 0x7c7b)
  MISMATCHED_STORE_TEST(133, sd, lw,  s0, 61, 61, 0x8786858483828180, SEXT(0x83828180, 32))
  MISMATCHED_STORE_TEST(134, sd, lw,  s0, 62, 62, 0x8f8e8d8c8b8a8988, SEXT(0x8b8a8988, 32))
  MISMATCHED_STORE_TEST(135, sd, lwu, s0, 63, 63, 0x9796959493929190, 0x93929190)
#endif

/* Memory contents at this point should be:
.word 0x10080000
.word 0x30282018
.word 0x34333231
.word 0x0f373635
.word 0x13121110
.word 0x17161514
.word 0x10080018
.word 0x30282018

.word 0x34333231
.word 0x27373635
.word 0x2b2a2928
.word 0x2f2e2d2c
.word 0x33323130
.word 0x37363534
.word 0x70686038
.word 0x90888078

.word 0x94939291
.word 0x47979695
.word 0x4b4a4948
.word 0x4f4e4d4c
.word 0x53525150
.word 0x57565554
.word 0x5b5a5958
.word 0x5f5e5d5c
.word 0x63626160
.word 0x67666564
.word 0x6b6a6968
.word 0x6f6e6d6c
.word 0x73727170
.word 0x77767574
.word 0x7b7a7978
.word 0x7f7e7d7c
*/

/* Check that a misaligned store writes the correct value, checked by a wider load. */

#if __riscv_xlen == 64
# within quadword
  MISMATCHED_STORE_TEST(136, sb, lh,  s0, 1, 1, 0x98, SEXT(0xb898, 16))
  MISMATCHED_STORE_TEST(137, sb, lhu, s0, 2, 1, 0x99, 0x9998)
  MISMATCHED_STORE_TEST(138, sh, lw,  s0, 1, 1, 0x9b9a, SEXT(0xc8c09b9a, 32))
  MISMATCHED_STORE_TEST(139, sh, lw,  s0, 3, 2, 0x9d9c, SEXT(0xd09d9c9b, 32))
  MISMATCHED_STORE_TEST(140, sh, lw,  s0, 5, 3, 0x9f9e, SEXT(0x9f9e9d9c, 32))

  MISMATCHED_STORE_TEST(141, sb, lwu, s0, 2, 1, 0xa0, 0x9d9ca09a)
  MISMATCHED_STORE_TEST(142, sh, lwu, s0, 3, 2, 0xa2a1, 0x9ea2a1a0)
  MISMATCHED_STORE_TEST(143, sh, lwu, s0, 5, 3, 0xa4a3, 0xa4a3a2a1)

  MISMATCHED_STORE_TEST(144, sb, ld, s0, 2,  1, 0xa5, 0xe1e0a4a3a2a1a59a)
  MISMATCHED_STORE_TEST(145, sh, ld, s0, 7,  2, 0xa7a6, 0xe2a7a6a4a3a2a1a5)
  MISMATCHED_STORE_TEST(146, sh, ld, s0, 9,  3, 0xa9a8, 0xa9a8a7a6a4a3a2a1)
  MISMATCHED_STORE_TEST(147, sw, ld, s0, 5,  4, 0xadacabaa, 0xe4a9a8adacabaaa2)
  MISMATCHED_STORE_TEST(148, sw, ld, s0, 7,  5, 0xb1b0afae, 0xe5e4b1b0afaeabaa)
  MISMATCHED_STORE_TEST(149, sw, ld, s0, 9,  6, 0xb5b4b3b2, 0xe6b5b4b3b2afaeab)
  MISMATCHED_STORE_TEST(150, sw, ld, s0, 11, 7, 0xb9b8b7b6, 0xb9b8b7b6b3b2afae)

# octword crossing
  MISMATCHED_STORE_TEST(151, sb, lh,  s0, 31, 31, 0xba, SEXT(0x31ba, 16))
  MISMATCHED_STORE_TEST(152, sb, lhu, s0, 32, 31, 0xbb, 0xbbba)
  MISMATCHED_STORE_TEST(153, sh, lw,  s0, 30, 30, 0xbdbc, SEXT(0x32bbbdbc, 32))
  MISMATCHED_STORE_TEST(154, sh, lw,  s0, 31, 30, 0xbfbe, SEXT(0x32bfbebc, 32))
  MISMATCHED_STORE_TEST(155, sh, lw,  s0, 32, 30, 0xc1c0, SEXT(0xc1c0bebc, 32))

  MISMATCHED_STORE_TEST(156, sb, lwu, s0, 32, 31, 0xc2, 0x33c1c2be)
  MISMATCHED_STORE_TEST(157, sh, lwu, s0, 31, 29, 0xc4c3, 0xc4c3bc20)
  MISMATCHED_STORE_TEST(158, sh, lwu, s0, 32, 30, 0xc6c5, 0xc6c5c3bc)

  MISMATCHED_STORE_TEST(159, sb, ld, s0, 32, 25, 0xc7, 0xc7c3bc2018100800)
  MISMATCHED_STORE_TEST(160, sh, ld, s0, 31, 26, 0xc9c8, 0xc6c9c8bc20181008)
  MISMATCHED_STORE_TEST(161, sh, ld, s0, 31, 27, 0xcbca, 0x33c6cbcabc201810)
  MISMATCHED_STORE_TEST(162, sw, ld, s0, 32, 28, 0xcfcecdcc, 0xcfcecdcccabc2018)
  MISMATCHED_STORE_TEST(163, sw, ld, s0, 31, 29, 0xd3d2d1d0, 0x35cfd3d2d1d0bc20)
  MISMATCHED_STORE_TEST(164, sw, ld, s0, 30, 30, 0xd7d6d5d4, 0x3635cfd3d7d6d5d4)
  MISMATCHED_STORE_TEST(165, sw, ld, s0, 29, 31, 0xdbdad9d8, 0x373635cfd3d7dbda)

# cacheline crossing
  MISMATCHED_STORE_TEST(166, sb, lh,  s0, 63, 63, 0xdc, SEXT(0x91dc, 16))
  MISMATCHED_STORE_TEST(167, sb, lhu, s0, 64, 63, 0xdd, 0xdddc)
  MISMATCHED_STORE_TEST(168, sh, lw,  s0, 62, 62, 0xdfde, SEXT(0x92dddfde, 32))
  MISMATCHED_STORE_TEST(169, sh, lw,  s0, 63, 62, 0xe1e0, SEXT(0x92e1e0de, 32))
  MISMATCHED_STORE_TEST(170, sh, lw,  s0, 64, 62, 0xe3e2, SEXT(0xe3e2e0de, 32))

  MISMATCHED_STORE_TEST(171, sb, lwu, s0, 64, 63, 0xe4, 0x93e3e4e0)
  MISMATCHED_STORE_TEST(172, sh, lwu, s0, 63, 61, 0xe6e5, 0xe6e5de80)
  MISMATCHED_STORE_TEST(173, sh, lwu, s0, 64, 62, 0xe8e7, 0xe8e7e5de)

  MISMATCHED_STORE_TEST(174, sb, ld, s0, 64, 57, 0xe9, 0xe9e5de8078706860)
  MISMATCHED_STORE_TEST(175, sh, ld, s0, 63, 58, 0xebea, 0xe8ebeade80787068)
  MISMATCHED_STORE_TEST(176, sh, ld, s0, 63, 59, 0xedec, 0x93e8edecde807870)
  MISMATCHED_STORE_TEST(177, sw, ld, s0, 64, 60, 0xf1f0efee, 0xf1f0efeeecde8078)
  MISMATCHED_STORE_TEST(178, sw, ld, s0, 63, 61, 0xf5f4f3f2, 0x95f1f5f4f3f2de80)
  MISMATCHED_STORE_TEST(179, sw, ld, s0, 62, 62, 0xf9f8f7f6, 0x9695f1f5f9f8f7f6)
  MISMATCHED_STORE_TEST(180, sw, ld, s0, 61, 63, 0xfdfcfbfa, 0x979695f1f5f9fdfc)
#endif

  TEST_PASSFAIL
FUNC_EPILOGUE


# ======================================================================
# rv32um: mul
# ======================================================================
FUNC_PROLOGUE test_rv32um_mul

  #-------------------------------------------------------------
  # Arithmetic tests
  #-------------------------------------------------------------

  TEST_RR_OP(43001,  mul, 0x00000000, 0x00000000, 0x00000000 );
  TEST_RR_OP(43002,  mul, 0x00000001, 0x00000001, 0x00000001 );
  TEST_RR_OP(43003,  mul, 0x00000015, 0x00000003, 0x00000007 );

  TEST_RR_OP(43004,  mul, 0x00000000, 0x00000000, 0xffff8000 );
  TEST_RR_OP(43005,  mul, 0x00000000, 0x80000000, 0x00000000 );
  TEST_RR_OP(43006,  mul, 0x00000000, 0x80000000, 0xffff8000 );

  #-------------------------------------------------------------
  # Source/Destination tests
  #-------------------------------------------------------------

  TEST_RR_SRC1_EQ_DEST(43007, mul, 143, 13, 11 );
  TEST_RR_SRC2_EQ_DEST(43008, mul, 154, 14, 11 );
  TEST_RR_SRC12_EQ_DEST(43009, mul, 169, 13 );

  #-------------------------------------------------------------
  # Bypassing tests
  #-------------------------------------------------------------

  TEST_RR_DEST_BYPASS(43010, 0, mul, 143, 13, 11 );
  TEST_RR_DEST_BYPASS(43011, 1, mul, 154, 14, 11 );
  TEST_RR_DEST_BYPASS(43012, 2, mul, 165, 15, 11 );

  TEST_RR_SRC12_BYPASS(43013, 0, 0, mul, 143, 13, 11 );
  TEST_RR_SRC12_BYPASS(43014, 0, 1, mul, 154, 14, 11 );
  TEST_RR_SRC12_BYPASS(43015, 0, 2, mul, 165, 15, 11 );
  TEST_RR_SRC12_BYPASS(43016, 1, 0, mul, 143, 13, 11 );
  TEST_RR_SRC12_BYPASS(43017, 1, 1, mul, 154, 14, 11 );
  TEST_RR_SRC12_BYPASS(43018, 2, 0, mul, 165, 15, 11 );

  TEST_RR_SRC21_BYPASS(43019, 0, 0, mul, 143, 13, 11 );
  TEST_RR_SRC21_BYPASS(43020, 0, 1, mul, 154, 14, 11 );
  TEST_RR_SRC21_BYPASS(43021, 0, 2, mul, 165, 15, 11 );
  TEST_RR_SRC21_BYPASS(43022, 1, 0, mul, 143, 13, 11 );
  TEST_RR_SRC21_BYPASS(43023, 1, 1, mul, 154, 14, 11 );
  TEST_RR_SRC21_BYPASS(43024, 2, 0, mul, 165, 15, 11 );

  TEST_RR_ZEROSRC1(43025, mul, 0, 31 );
  TEST_RR_ZEROSRC2(43026, mul, 0, 32 );
  TEST_RR_ZEROSRC12(43027, mul, 0 );
  TEST_RR_ZERODEST(43028, mul, 33, 34 );

  #-------------------------------------------------------------
  # Additional arithmetic tests
  #-------------------------------------------------------------

  TEST_RR_OP(43029,  mul, 0x0000ff7f, 0xaaaaaaab, 0x0002fe7d );
  TEST_RR_OP(43030,  mul, 0x0000ff7f, 0x0002fe7d, 0xaaaaaaab );

  TEST_RR_OP(43031,  mul, 0x00001200, 0x00007e00, 0xb6db6db7 );
  TEST_RR_OP(43032,  mul, 0x00001240, 0x00007fc0, 0xb6db6db7 );

  TEST_RR_OP(43033,  mul, 0x00000000, 0xff000000, 0xff000000 );

  TEST_RR_OP(43034,  mul, 0x00000001, 0xffffffff, 0xffffffff );
  TEST_RR_OP(43035,  mul, 0xffffffff, 0xffffffff, 0x00000001 );
  TEST_RR_OP(43036,  mul, 0xffffffff, 0x00000001, 0xffffffff );

  TEST_PASSFAIL
FUNC_EPILOGUE


# ======================================================================
# rv32um: mulh
# ======================================================================
FUNC_PROLOGUE test_rv32um_mulh

  #-------------------------------------------------------------
  # Arithmetic tests
  #-------------------------------------------------------------

  TEST_RR_OP(44001,  mulh, 0x00000000, 0x00000000, 0x00000000 );
  TEST_RR_OP(44002,  mulh, 0x00000000, 0x00000001, 0x00000001 );
  TEST_RR_OP(44003,  mulh, 0x00000000, 0x00000003, 0x00000007 );

  TEST_RR_OP(44004,  mulh, 0x00000000, 0x00000000, 0xffff8000 );
  TEST_RR_OP(44005,  mulh, 0x00000000, 0x80000000, 0x00000000 );
  TEST_RR_OP(44006,  mulh, 0x00004000, 0x80000000, 0xffff8000 );

  #-------------------------------------------------------------
  # Source/Destination tests
  #-------------------------------------------------------------

  TEST_RR_SRC1_EQ_DEST(44007, mulh, 36608, 13<<20, 11<<20 );
  TEST_RR_SRC2_EQ_DEST(44008, mulh, 39424, 14<<20, 11<<20 );
  TEST_RR_SRC12_EQ_DEST(44009, mulh, 43264, 13<<20 );

  #-------------------------------------------------------------
  # Bypassing tests
  #-------------------------------------------------------------

  TEST_RR_DEST_BYPASS(44010, 0, mulh, 36608, 13<<20, 11<<20 );
  TEST_RR_DEST_BYPASS(44011, 1, mulh, 39424, 14<<20, 11<<20 );
  TEST_RR_DEST_BYPASS(44012, 2, mulh, 42240, 15<<20, 11<<20 );

  TEST_RR_SRC12_BYPASS(44013, 0, 0, mulh, 36608, 13<<20, 11<<20 );
  TEST_RR_SRC12_BYPASS(44014, 0, 1, mulh, 39424, 14<<20, 11<<20 );
  TEST_RR_SRC12_BYPASS(44015, 0, 2, mulh, 42240, 15<<20, 11<<20 );
  TEST_RR_SRC12_BYPASS(44016, 1, 0, mulh, 36608, 13<<20, 11<<20 );
  TEST_RR_SRC12_BYPASS(44017, 1, 1, mulh, 39424, 14<<20, 11<<20 );
  TEST_RR_SRC12_BYPASS(44018, 2, 0, mulh, 42240, 15<<20, 11<<20 );

  TEST_RR_SRC21_BYPASS(44019, 0, 0, mulh, 36608, 13<<20, 11<<20 );
  TEST_RR_SRC21_BYPASS(44020, 0, 1, mulh, 39424, 14<<20, 11<<20 );
  TEST_RR_SRC21_BYPASS(44021, 0, 2, mulh, 42240, 15<<20, 11<<20 );
  TEST_RR_SRC21_BYPASS(44022, 1, 0, mulh, 36608, 13<<20, 11<<20 );
  TEST_RR_SRC21_BYPASS(44023, 1, 1, mulh, 39424, 14<<20, 11<<20 );
  TEST_RR_SRC21_BYPASS(44024, 2, 0, mulh, 42240, 15<<20, 11<<20 );

  TEST_RR_ZEROSRC1(44025, mulh, 0, 31<<26 );
  TEST_RR_ZEROSRC2(44026, mulh, 0, 32<<26 );
  TEST_RR_ZEROSRC12(44027, mulh, 0 );
  TEST_RR_ZERODEST(44028, mulh, 33<<20, 34<<20 );

  #-------------------------------------------------------------
  # Additional arithmetic tests
  #-------------------------------------------------------------

  TEST_RR_OP(44029,  mulh, 0xffff0081, 0xaaaaaaab, 0x0002fe7d );
  TEST_RR_OP(44030,  mulh, 0xffff0081, 0x0002fe7d, 0xaaaaaaab );

  TEST_RR_OP(44031,  mulh, 0x00010000, 0xff000000, 0xff000000 );

  TEST_RR_OP(44032,  mulh, 0x00000000, 0xffffffff, 0xffffffff );
  TEST_RR_OP(44033,  mulh, 0xffffffff, 0xffffffff, 0x00000001 );
  TEST_RR_OP(44034,  mulh, 0xffffffff, 0x00000001, 0xffffffff );

  TEST_PASSFAIL
FUNC_EPILOGUE


# ======================================================================
# rv32um: mulhsu
# ======================================================================
FUNC_PROLOGUE test_rv32um_mulhsu

  #-------------------------------------------------------------
  # Arithmetic tests
  #-------------------------------------------------------------

  TEST_RR_OP(45001,  mulhsu, 0x00000000, 0x00000000, 0x00000000 );
  TEST_RR_OP(45002,  mulhsu, 0x00000000, 0x00000001, 0x00000001 );
  TEST_RR_OP(45003,  mulhsu, 0x00000000, 0x00000003, 0x00000007 );

  TEST_RR_OP(45004,  mulhsu, 0x00000000, 0x00000000, 0xffff8000 );
  TEST_RR_OP(45005,  mulhsu, 0x00000000, 0x80000000, 0x00000000 );
  TEST_RR_OP(45006,  mulhsu, 0x80004000, 0x80000000, 0xffff8000 );

  #-------------------------------------------------------------
  # Source/Destination tests
  #-------------------------------------------------------------

  TEST_RR_SRC1_EQ_DEST(45007, mulhsu, 36608, 13<<20, 11<<20 );
  TEST_RR_SRC2_EQ_DEST(45008, mulhsu, 39424, 14<<20, 11<<20 );
  TEST_RR_SRC12_EQ_DEST(45009, mulhsu, 43264, 13<<20 );

  #-------------------------------------------------------------
  # Bypassing tests
  #-------------------------------------------------------------

  TEST_RR_DEST_BYPASS(45010, 0, mulhsu, 36608, 13<<20, 11<<20 );
  TEST_RR_DEST_BYPASS(45011, 1, mulhsu, 39424, 14<<20, 11<<20 );
  TEST_RR_DEST_BYPASS(45012, 2, mulhsu, 42240, 15<<20, 11<<20 );

  TEST_RR_SRC12_BYPASS(45013, 0, 0, mulhsu, 36608, 13<<20, 11<<20 );
  TEST_RR_SRC12_BYPASS(45014, 0, 1, mulhsu, 39424, 14<<20, 11<<20 );
  TEST_RR_SRC12_BYPASS(45015, 0, 2, mulhsu, 42240, 15<<20, 11<<20 );
  TEST_RR_SRC12_BYPASS(45016, 1, 0, mulhsu, 36608, 13<<20, 11<<20 );
  TEST_RR_SRC12_BYPASS(45017, 1, 1, mulhsu, 39424, 14<<20, 11<<20 );
  TEST_RR_SRC12_BYPASS(45018, 2, 0, mulhsu, 42240, 15<<20, 11<<20 );

  TEST_RR_SRC21_BYPASS(45019, 0, 0, mulhsu, 36608, 13<<20, 11<<20 );
  TEST_RR_SRC21_BYPASS(45020, 0, 1, mulhsu, 39424, 14<<20, 11<<20 );
  TEST_RR_SRC21_BYPASS(45021, 0, 2, mulhsu, 42240, 15<<20, 11<<20 );
  TEST_RR_SRC21_BYPASS(45022, 1, 0, mulhsu, 36608, 13<<20, 11<<20 );
  TEST_RR_SRC21_BYPASS(45023, 1, 1, mulhsu, 39424, 14<<20, 11<<20 );
  TEST_RR_SRC21_BYPASS(45024, 2, 0, mulhsu, 42240, 15<<20, 11<<20 );

  TEST_RR_ZEROSRC1(45025, mulhsu, 0, 31<<26 );
  TEST_RR_ZEROSRC2(45026, mulhsu, 0, 32<<26 );
  TEST_RR_ZEROSRC12(45027, mulhsu, 0 );
  TEST_RR_ZERODEST(45028, mulhsu, 33<<20, 34<<20 );

  #-------------------------------------------------------------
  # Additional arithmetic tests
  #-------------------------------------------------------------

  TEST_RR_OP(45029,  mulhsu, 0xffff0081, 0xaaaaaaab, 0x0002fe7d );
  TEST_RR_OP(45030,  mulhsu, 0x0001fefe, 0x0002fe7d, 0xaaaaaaab );

  TEST_RR_OP(45031,  mulhsu, 0xff010000, 0xff000000, 0xff000000 );

  TEST_RR_OP(45032,  mulhsu, 0xffffffff, 0xffffffff, 0xffffffff );
  TEST_RR_OP(45033,  mulhsu, 0xffffffff, 0xffffffff, 0x00000001 );
  TEST_RR_OP(45034,  mulhsu, 0x00000000, 0x00000001, 0xffffffff );

  TEST_PASSFAIL
FUNC_EPILOGUE


# ======================================================================
# rv32um: mulhu
# ======================================================================
FUNC_PROLOGUE test_rv32um_mulhu

  #-------------------------------------------------------------
  # Arithmetic tests
  #-------------------------------------------------------------

  TEST_RR_OP(46001,  mulhu, 0x00000000, 0x00000000, 0x00000000 );
  TEST_RR_OP(46002,  mulhu, 0x00000000, 0x00000001, 0x00000001 );
  TEST_RR_OP(46003,  mulhu, 0x00000000, 0x00000003, 0x00000007 );

  TEST_RR_OP(46004,  mulhu, 0x00000000, 0x00000000, 0xffff8000 );
  TEST_RR_OP(46005,  mulhu, 0x00000000, 0x80000000, 0x00000000 );
  TEST_RR_OP(46006,  mulhu, 0x7fffc000, 0x80000000, 0xffff8000 );

  #-------------------------------------------------------------
  # Source/Destination tests
  #-------------------------------------------------------------

  TEST_RR_SRC1_EQ_DEST(46007, mulhu, 36608, 13<<20, 11<<20 );
  TEST_RR_SRC2_EQ_DEST(46008, mulhu, 39424, 14<<20, 11<<20 );
  TEST_RR_SRC12_EQ_DEST(46009, mulhu, 43264, 13<<20 );

  #-------------------------------------------------------------
  # Bypassing tests
  #-------------------------------------------------------------

  TEST_RR_DEST_BYPASS(46010, 0, mulhu, 36608, 13<<20, 11<<20 );
  TEST_RR_DEST_BYPASS(46011, 1, mulhu, 39424, 14<<20, 11<<20 );
  TEST_RR_DEST_BYPASS(46012, 2, mulhu, 42240, 15<<20, 11<<20 );

  TEST_RR_SRC12_BYPASS(46013, 0, 0, mulhu, 36608, 13<<20, 11<<20 );
  TEST_RR_SRC12_BYPASS(46014, 0, 1, mulhu, 39424, 14<<20, 11<<20 );
  TEST_RR_SRC12_BYPASS(46015, 0, 2, mulhu, 42240, 15<<20, 11<<20 );
  TEST_RR_SRC12_BYPASS(46016, 1, 0, mulhu, 36608, 13<<20, 11<<20 );
  TEST_RR_SRC12_BYPASS(46017, 1, 1, mulhu, 39424, 14<<20, 11<<20 );
  TEST_RR_SRC12_BYPASS(46018, 2, 0, mulhu, 42240, 15<<20, 11<<20 );

  TEST_RR_SRC21_BYPASS(46019, 0, 0, mulhu, 36608, 13<<20, 11<<20 );
  TEST_RR_SRC21_BYPASS(46020, 0, 1, mulhu, 39424, 14<<20, 11<<20 );
  TEST_RR_SRC21_BYPASS(46021, 0, 2, mulhu, 42240, 15<<20, 11<<20 );
  TEST_RR_SRC21_BYPASS(46022, 1, 0, mulhu, 36608, 13<<20, 11<<20 );
  TEST_RR_SRC21_BYPASS(46023, 1, 1, mulhu, 39424, 14<<20, 11<<20 );
  TEST_RR_SRC21_BYPASS(46024, 2, 0, mulhu, 42240, 15<<20, 11<<20 );

  TEST_RR_ZEROSRC1(46025, mulhu, 0, 31<<26 );
  TEST_RR_ZEROSRC2(46026, mulhu, 0, 32<<26 );
  TEST_RR_ZEROSRC12(46027, mulhu, 0 );
  TEST_RR_ZERODEST(46028, mulhu, 33<<20, 34<<20 );

  #-------------------------------------------------------------
  # Additional arithmetic tests
  #-------------------------------------------------------------

  TEST_RR_OP(46029,  mulhu, 0x0001fefe, 0xaaaaaaab, 0x0002fe7d );
  TEST_RR_OP(46030,  mulhu, 0x0001fefe, 0x0002fe7d, 0xaaaaaaab );

  TEST_RR_OP(46031,  mulhu, 0xfe010000, 0xff000000, 0xff000000 );

  TEST_RR_OP(46032,  mulhu, 0xfffffffe, 0xffffffff, 0xffffffff );
  TEST_RR_OP(46033,  mulhu, 0x00000000, 0xffffffff, 0x00000001 );
  TEST_RR_OP(46034,  mulhu, 0x00000000, 0x00000001, 0xffffffff );

  TEST_PASSFAIL
FUNC_EPILOGUE


# ======================================================================
# rv32um: div
# ======================================================================
FUNC_PROLOGUE test_rv32um_div

  #-------------------------------------------------------------
  # Arithmetic tests
  #-------------------------------------------------------------

  TEST_RR_OP(47001, div,  3,  20,   6 );
  TEST_RR_OP(47002, div, -3, -20,   6 );
  TEST_RR_OP(47003, div, -3,  20,  -6 );
  TEST_RR_OP(47004, div,  3, -20,  -6 );

  TEST_RR_OP(47005, div, -1<<31, -1<<31,  1 );
  TEST_RR_OP(47006, div, -1<<31, -1<<31, -1 );

  TEST_RR_OP(47007, div, -1, -1<<31, 0 );
  TEST_RR_OP(47008, div, -1,      1, 0 );
  TEST_RR_OP(47009, div, -1,      0, 0 );

  TEST_PASSFAIL
FUNC_EPILOGUE


# ======================================================================
# rv32um: divu
# ======================================================================
FUNC_PROLOGUE test_rv32um_divu

  #-------------------------------------------------------------
  # Arithmetic tests
  #-------------------------------------------------------------

  TEST_RR_OP(48001, divu,                   3,  20,   6 );
  TEST_RR_OP(48002, divu,           715827879, -20,   6 );
  TEST_RR_OP(48003, divu,                   0,  20,  -6 );
  TEST_RR_OP(48004, divu,                   0, -20,  -6 );

  TEST_RR_OP(48005, divu, -1<<31, -1<<31,  1 );
  TEST_RR_OP(48006, divu,     0,  -1<<31, -1 );

  TEST_RR_OP(48007, divu, -1, -1<<31, 0 );
  TEST_RR_OP(48008, divu, -1,      1, 0 );
  TEST_RR_OP(48009, divu, -1,      0, 0 );

  TEST_PASSFAIL
FUNC_EPILOGUE


# ======================================================================
# rv32um: rem
# ======================================================================
FUNC_PROLOGUE test_rv32um_rem

  #-------------------------------------------------------------
  # Arithmetic tests
  #-------------------------------------------------------------

  TEST_RR_OP(49001, rem,  2,  20,   6 );
  TEST_RR_OP(49002, rem, -2, -20,   6 );
  TEST_RR_OP(49003, rem,  2,  20,  -6 );
  TEST_RR_OP(49004, rem, -2, -20,  -6 );

  TEST_RR_OP(49005, rem,  0, -1<<31,  1 );
  TEST_RR_OP(49006, rem,  0, -1<<31, -1 );

  TEST_RR_OP(49007, rem, -1<<31, -1<<31, 0 );
  TEST_RR_OP(49008, rem,      1,      1, 0 );
  TEST_RR_OP(49009, rem,      0,      0, 0 );

  TEST_PASSFAIL
FUNC_EPILOGUE


# ======================================================================
# rv32um: remu
# ======================================================================
FUNC_PROLOGUE test_rv32um_remu

  #-------------------------------------------------------------
  # Arithmetic tests
  #-------------------------------------------------------------

  TEST_RR_OP(50001, remu,   2,  20,   6 );
  TEST_RR_OP(50002, remu,   2, -20,   6 );
  TEST_RR_OP(50003, remu,  20,  20,  -6 );
  TEST_RR_OP(50004, remu, -20, -20,  -6 );

  TEST_RR_OP(50005, remu,      0, -1<<31,  1 );
  TEST_RR_OP(50006, remu, -1<<31, -1<<31, -1 );

  TEST_RR_OP(50007, remu, -1<<31, -1<<31, 0 );
  TEST_RR_OP(50008, remu,      1,      1, 0 );
  TEST_RR_OP(50009, remu,      0,      0, 0 );

  TEST_PASSFAIL
FUNC_EPILOGUE


# ======================================================================
# rv32ua: amoadd_w
# ======================================================================
FUNC_PROLOGUE test_rv32ua_amoadd_w

  TEST_CASE(51001, a4, 0xffffffff80000000, \
    li a0, 0xffffffff80000000; \
    li a1, 0xfffffffffffff800; \
    la a3, amo_operand_amoadd_w; \
    sw a0, 0(a3); \
    amoadd.w	a4, a1, 0(a3); \
  )

  TEST_CASE(51002, a5, 0x000000007ffff800, lw a5, 0(a3))

  # try again after a cache miss
  TEST_CASE(51003, a4, 0x000000007ffff800, \
    li  a1, 0xffffffff80000000; \
    amoadd.w a4, a1, 0(a3); \
  )

  TEST_CASE(51004, a5, 0xfffffffffffff800, lw a5, 0(a3))

  TEST_PASSFAIL
FUNC_EPILOGUE


# ======================================================================
# rv32ua: amoand_w
# ======================================================================
FUNC_PROLOGUE test_rv32ua_amoand_w

  TEST_CASE(52001, a4, 0xffffffff80000000, \
    li a0, 0xffffffff80000000; \
    li a1, 0xfffffffffffff800; \
    la a3, amo_operand_amoand_w; \
    sw a0, 0(a3); \
    amoand.w	a4, a1, 0(a3); \
  )

  TEST_CASE(52002, a5, 0xffffffff80000000, lw a5, 0(a3))

  # try again after a cache miss
  TEST_CASE(52003, a4, 0xffffffff80000000, \
    li  a1, 0x0000000080000000; \
    amoand.w a4, a1, 0(a3); \
  )

  TEST_CASE(52004, a5, 0xffffffff80000000, lw a5, 0(a3))

  TEST_PASSFAIL
FUNC_EPILOGUE


# ======================================================================
# rv32ua: amomax_w
# ======================================================================
FUNC_PROLOGUE test_rv32ua_amomax_w

  TEST_CASE(53001, a4, 0xffffffff80000000, \
    li a0, 0xffffffff80000000; \
    li a1, 0xfffffffffffff800; \
    la a3, amo_operand_amomax_w; \
    sw a0, 0(a3); \
    amomax.w	a4, a1, 0(a3); \
  )

  TEST_CASE(53002, a5, 0xfffffffffffff800, lw a5, 0(a3))

  TEST_CASE(53003, a4, 0, \
    li a1, 1; \
    sw x0, 0(a3); \
    amomax.w	a4, a1, 0(a3); \
  )

  TEST_CASE(53004, a5, 1, lw a5, 0(a3))

  TEST_CASE(53005, a4, 1, \
    li a0, 0x0000000000000001; \
    li a1, 0x0000000080000000; \
    la a3, amo_operand_amomax_w; \
    sw a0, 0(a3); \
    amomax.w	a4, a1, 0(a3); \
  )

  TEST_CASE(53006, a5, 1, lw a5, 0(a3))

  TEST_PASSFAIL
FUNC_EPILOGUE


# ======================================================================
# rv32ua: amomaxu_w
# ======================================================================
FUNC_PROLOGUE test_rv32ua_amomaxu_w

  TEST_CASE(54001, a4, 0xffffffff80000000, \
    li a0, 0xffffffff80000000; \
    li a1, 0xfffffffffffff800; \
    la a3, amo_operand_amomaxu_w; \
    sw a0, 0(a3); \
    amomaxu.w	a4, a1, 0(a3); \
  )

  TEST_CASE(54002, a5, 0xfffffffffffff800, lw a5, 0(a3))

  TEST_CASE(54003, a4, 0, \
    li a1, 0xffffffffffffffff; \
    sw x0, 0(a3); \
    amomaxu.w	a4, a1, 0(a3); \
  )

  TEST_CASE(54004, a5, 0xffffffffffffffff, lw a5, 0(a3))

  # The following two cases check that the upper 32 bits are ignored on rv64
  # implementations. Only enable them for rv64.
  #if __riscv_xlen == 64
  TEST_CASE(54005, a4, 1, \
    li a0, 0x0000000000000001; \
    li a1, 0x8000000000000000; \
    la a3, amo_operand_amomaxu_w; \
    sw a0, 0(a3); \
    amomaxu.w	a4, a1, 0(a3); \
  )

  TEST_CASE(54006, a5, 1, lw a5, 0(a3))
  #endif

  TEST_PASSFAIL
FUNC_EPILOGUE


# ======================================================================
# rv32ua: amomin_w
# ======================================================================
FUNC_PROLOGUE test_rv32ua_amomin_w

  TEST_CASE(55001, a4, 0xffffffff80000000, \
    li a0, 0xffffffff80000000; \
    li a1, 0xfffffffffffff800; \
    la a3, amo_operand_amomin_w; \
    sw a0, 0(a3); \
    amomin.w	a4, a1, 0(a3); \
  )

  TEST_CASE(55002, a5, 0xffffffff80000000, lw a5, 0(a3))

  TEST_CASE(55003, a4, 0, \
    li a1, 0xffffffffffffffff; \
    sw x0, 0(a3); \
    amomin.w	a4, a1, 0(a3); \
  )

  TEST_CASE(55004, a5, 0xffffffffffffffff, lw a5, 0(a3))

  TEST_CASE(55005, a4, 1, \
    li a0, 0x0000000000000001; \
    li a1, 0x0000000080000000; \
    la a3, amo_operand_amomin_w; \
    sw a0, 0(a3); \
    amomin.w	a4, a1, 0(a3); \
  )

  TEST_CASE(55006, a5, 0xffffffff80000000, lw a5, 0(a3))

  TEST_PASSFAIL
FUNC_EPILOGUE


# ======================================================================
# rv32ua: amominu_w
# ======================================================================
FUNC_PROLOGUE test_rv32ua_amominu_w

  TEST_CASE(56001, a4, 0xffffffff80000000, \
    li a0, 0xffffffff80000000; \
    li a1, 0xfffffffffffff800; \
    la a3, amo_operand_amominu_w; \
    sw a0, 0(a3); \
    amominu.w	a4, a1, 0(a3); \
  )

  TEST_CASE(56002, a5, 0xffffffff80000000, lw a5, 0(a3))

  TEST_CASE(56003, a4, 0, \
    li a1, 0xffffffffffffffff; \
    sw x0, 0(a3); \
    amominu.w	a4, a1, 0(a3); \
  )

  TEST_CASE(56004, a5, 0, lw a5, 0(a3))

  # The following two cases check that the upper 32 bits are ignored on rv64
  # implementations. Only enable them for rv64.
  #if __riscv_xlen == 64
  TEST_CASE(56005, a4, 1, \
    li a0, 0x0000000000000001; \
    li a1, 0x8000000000000000; \
    la a3, amo_operand_amominu_w; \
    sw a0, 0(a3); \
    amominu.w	a4, a1, 0(a3); \
  )

  TEST_CASE(56006, a5, 0, lw a5, 0(a3))
  #endif

  TEST_PASSFAIL
FUNC_EPILOGUE


# ======================================================================
# rv32ua: amoor_w
# ======================================================================
FUNC_PROLOGUE test_rv32ua_amoor_w

  TEST_CASE(57001, a4, 0xffffffff80000000, \
    li a0, 0xffffffff80000000; \
    li a1, 0xfffffffffffff800; \
    la a3, amo_operand_amoor_w; \
    sw a0, 0(a3); \
    amoor.w	a4, a1, 0(a3); \
  )

  TEST_CASE(57002, a5, 0xfffffffffffff800, lw a5, 0(a3))

  # try again after a cache miss
  TEST_CASE(57003, a4, 0xfffffffffffff800, \
    li  a1, 1; \
    amoor.w a4, a1, 0(a3); \
  )

  TEST_CASE(57004, a5, 0xfffffffffffff801, lw a5, 0(a3))

  TEST_PASSFAIL
FUNC_EPILOGUE


# ======================================================================
# rv32ua: amoxor_w
# ======================================================================
FUNC_PROLOGUE test_rv32ua_amoxor_w

  TEST_CASE(58001, a4, 0xffffffff80000000, \
    li a0, 0xffffffff80000000; \
    li a1, 0xfffffffffffff800; \
    la a3, amo_operand_amoxor_w; \
    sw a0, 0(a3); \
    amoxor.w	a4, a1, 0(a3); \
  )

  TEST_CASE(58002, a5, 0x7ffff800, lw a5, 0(a3))

  # try again after a cache miss
  TEST_CASE(58003, a4, 0x7ffff800, \
    li  a1, 0xc0000001; \
    amoxor.w a4, a1, 0(a3); \
  )

  TEST_CASE(58004, a5, 0xffffffffbffff801, lw a5, 0(a3))

  TEST_PASSFAIL
FUNC_EPILOGUE


# ======================================================================
# rv32ua: amoswap_w
# ======================================================================
FUNC_PROLOGUE test_rv32ua_amoswap_w

  TEST_CASE(59001, a4, 0xffffffff80000000, \
    li a0, 0xffffffff80000000; \
    li a1, 0xfffffffffffff800; \
    la a3, amo_operand_amoswap_w; \
    sw a0, 0(a3); \
    amoswap.w	a4, a1, 0(a3); \
  )

  TEST_CASE(59002, a5, 0xfffffffffffff800, lw a5, 0(a3))

  # try again after a cache miss
  TEST_CASE(59003, a4, 0xfffffffffffff800, \
    li  a1, 0x0000000080000000; \
    amoswap.w a4, a1, 0(a3); \
  )

  TEST_CASE(59004, a5, 0xffffffff80000000, lw a5, 0(a3))

  TEST_PASSFAIL
FUNC_EPILOGUE


# ======================================================================
# rv32ua: lrsc
# ======================================================================
FUNC_PROLOGUE test_rv32ua_lrsc

# get a unique core id
la a0, coreid_lrsc
li a1, 1
amoadd.w a2, a1, (a0)

# for now, only run this on core 0
1:li a3, 1
bgeu a2, a3, 1b

1: lw a1, (a0)
bltu a1, a3, 1b

# make sure that sc without a reservation fails.
TEST_CASE(60001, a4, 1, \
  la a0, foo_lrsc; \
  li a5, 0xdeadbeef; \
  sc.w a4, a5, (a0); \
)

# make sure the failing sc did not commit into memory
TEST_CASE(60002, a4, 0, \
  lw a4, foo_lrsc; \
)

#
# Disable test case 4 for now. It assumes a <1K reservation granule, when
# in reality any size granule is valid. After discussion in issue #315,
# decided to simply disable the test for now.
# (See https://github.com/riscv/riscv-tests/issues/315)
#
## make sure that sc with the wrong reservation fails.
## TODO is this actually mandatory behavior?
#TEST_CASE(60003, a4, 1, \
#  la a0, foo_lrsc; \
#  la a1, fooTest3_lrsc; \
#  lr.w a1, (a1); \
#  sc.w a4, a1, (a0); \
#)

#define LOG_ITERATIONS 10

# have each core add its coreid_lrsc+1 to foo_lrsc 1024 times
la a0, foo_lrsc
li a1, 1<<LOG_ITERATIONS
addi a2, a2, 1
1: lr.w a4, (a0)
add a4, a4, a2
sc.w a4, a4, (a0)
bnez a4, 1b
addi a1, a1, -1
bnez a1, 1b

# wait for all cores to finish
la a0, barrier_lrsc
li a1, 1
amoadd.w x0, a1, (a0)
1: lw a1, (a0)
blt a1, a3, 1b
fence

# expected result is 512*ncores*(ncores+1)
TEST_CASE(60004, a0, 0, \
  lw a0, foo_lrsc; \
  slli a1, a3, LOG_ITERATIONS-1; \
1:sub a0, a0, a1; \
  addi a3, a3, -1; \
  bgez a3, 1b
)

# make sure that sc-after-successful-sc fails.
TEST_CASE(60005, a1, 2, \
  la a0, foo_lrsc; \
1:lr.w a1, (a0); \
  sc.w a1, x0, (a0); \
  bnez a1, 1b; \
  sc.w a1, x0, (a0); \
  /* make sure that sc-after-failed-sc fails, too */ \
  sc.w a2, x0, (a0); \
  add a1, a1, a2
)

  TEST_PASSFAIL
FUNC_EPILOGUE

# ======================================================================
# rv32uc: rvc
# ======================================================================
FUNC_PROLOGUE test_rv32uc_rvc

  .align 2
  .option push
  .option norvc

  #define RVC_TEST_CASE(n, r, v, code...) \
    TEST_CASE (n, r, v, .option push; .option rvc; code; .align 2; .option pop)

  // Make sure fetching a 4-byte instruction across a page boundary works.
  li TESTNUM, 61001
  li a1, 666
  TEST_CASE(61001, a1, 667, addi a1, a1, 1)

  li sp, 0x1234
  RVC_TEST_CASE(61002, a0, 0x1234 + 1020, c.addi4spn a0, sp, 1020)
  RVC_TEST_CASE(61003, sp, 0x1234 + 496, c.addi16sp sp, 496)
  RVC_TEST_CASE(61004, sp, 0x1234 + 496 - 512, c.addi16sp sp, -512)

  la a1, data_rvc
  li a0, 0x76543210
  sw a0, 0(a1)
  sw a0, 8(a1)
  li a0, 0xfedcba98
  sw a0, 4(a1)
  sw a0, 12(a1)
  RVC_TEST_CASE(61005, a2, 0xfffffffffedcba99, c.lw a0, 4(a1); addi a0, a0, 1; c.sw a0, 4(a1); c.lw a2, 4(a1))
#if __riscv_xlen == 64
  RVC_TEST_CASE(61006, a2, 0xfedcba9976543211, c.ld a0, 0(a1); addi a0, a0, 1; c.sd a0, 0(a1); c.ld a2, 0(a1))
#endif

  RVC_TEST_CASE(61007, a0, -15, ori a0, x0, 1; c.addi a0, -16)
  RVC_TEST_CASE(61008, a5, -16, ori a5, x0, 1; c.li a5, -16)
#if __riscv_xlen == 64
  RVC_TEST_CASE(61009, a0, 0x76543210, ld a0, (a1); c.addiw a0, -1)
#endif

  RVC_TEST_CASE(61010, s0, 0xffffffffffffffe1, c.lui s0, 0xfffe1; c.srai s0, 12)
#if __riscv_xlen == 64
  RVC_TEST_CASE(61011, s0, 0x000fffffffffffe1, c.lui s0, 0xfffe1; c.srli s0, 12)
#else
  RVC_TEST_CASE(61012, s0, 0x000fffe1, c.lui s0, 0xfffe1; c.srli s0, 12)
#endif
  RVC_TEST_CASE(61013, s0, ~0x11, c.li s0, -2; c.andi s0, ~0x10)
  RVC_TEST_CASE(61014, s1, 14, li s1, 20; li a0, 6; c.sub s1, a0)
  RVC_TEST_CASE(61015, s1, 18, li s1, 20; li a0, 6; c.xor s1, a0)
  RVC_TEST_CASE(61016, s1, 22, li s1, 20; li a0, 6; c.or s1, a0)
  RVC_TEST_CASE(61017, s1,  4, li s1, 20; li a0, 6; c.and s1, a0)
#if __riscv_xlen == 64
  RVC_TEST_CASE(61018, s1, 0xffffffff80000000, li s1, 0x7fffffff; li a0, -1; c.subw s1, a0)
  RVC_TEST_CASE(61019, s1, 0xffffffff80000000, li s1, 0x7fffffff; li a0, 1; c.addw s1, a0)
#endif
  RVC_TEST_CASE(61020, s0, 0x12340, li s0, 0x1234; c.slli s0, 4)

  RVC_TEST_CASE(61021, ra, 0, \
        li ra, 0; \
        c.j 1f; \
        c.j 2f; \
      1:c.j 1f; \
      2:j fail; \
      1:)

  RVC_TEST_CASE(61022, x0, 0, \
        li a0, 0; \
        c.beqz a0, 1f; \
        c.j 2f; \
      1:c.j 1f; \
      2:j fail; \
      1:)

  RVC_TEST_CASE(61023, x0, 0, \
        li a0, 1; \
        c.bnez a0, 1f; \
        c.j 2f; \
      1:c.j 1f; \
      2:j fail; \
      1:)

  RVC_TEST_CASE(61024, x0, 0, \
        li a0, 1; \
        c.beqz a0, 1f; \
        c.j 2f; \
      1:j fail; \
      2:)

  RVC_TEST_CASE(61025, x0, 0, \
        li a0, 0; \
        c.bnez a0, 1f; \
        c.j 2f; \
      1:j fail; \
      2:)

  RVC_TEST_CASE(61026, ra, 0, \
        la t0, 1f; \
        li ra, 0; \
        c.jr t0; \
        c.j 2f; \
      1:c.j 1f; \
      2:j fail; \
      1:)

  RVC_TEST_CASE(61027, ra, -2, \
        la t0, 1f; \
        li ra, 0; \
        c.jalr t0; \
        c.j 2f; \
      1:c.j 1f; \
      2:j fail; \
      1:sub ra, ra, t0)

#if __riscv_xlen == 32
  RVC_TEST_CASE(61028, ra, -2, \
        la t0, 1f; \
        li ra, 0; \
        c.jal 1f; \
        c.j 2f; \
      1:c.j 1f; \
      2:j fail; \
      1:sub ra, ra, t0)
#endif

  la sp, data_rvc
  RVC_TEST_CASE(61029, a2, 0xfffffffffedcba99, c.lwsp a0, 12(sp); addi a0, a0, 1; c.swsp a0, 12(sp); c.lwsp a2, 12(sp))
#if __riscv_xlen == 64
  RVC_TEST_CASE(61030, a2, 0xfedcba9976543211, c.ldsp a0, 8(sp); addi a0, a0, 1; c.sdsp a0, 8(sp); c.ldsp a2, 8(sp))
#endif

  RVC_TEST_CASE(61031, t0, 0x246, li a0, 0x123; c.mv t0, a0; c.add t0, a0)

  .option pop

  TEST_PASSFAIL
FUNC_EPILOGUE


# ======================================================================
# rv32uf: fadd
# ======================================================================
FUNC_PROLOGUE test_rv32uf_fadd

  #-------------------------------------------------------------
  # Arithmetic tests
  #-------------------------------------------------------------

  TEST_FP_OP2_S(62001,  fadd.s, 0,                3.5,        2.5,        1.0 );
  TEST_FP_OP2_S(62002,  fadd.s, 1,              -1234,    -1235.1,        1.1 );
  TEST_FP_OP2_S(62003,  fadd.s, 1,         3.14159265, 3.14159265, 0.00000001 );

  TEST_FP_OP2_S(62004,  fsub.s, 0,                1.5,        2.5,        1.0 );
  TEST_FP_OP2_S(62005,  fsub.s, 1,              -1234,    -1235.1,       -1.1 );
  TEST_FP_OP2_S(62006,  fsub.s, 1,         3.14159265, 3.14159265, 0.00000001 );

  TEST_FP_OP2_S(62007,  fmul.s, 0,                2.5,        2.5,        1.0 );
  TEST_FP_OP2_S(62008,  fmul.s, 1,            1358.61,    -1235.1,       -1.1 );
  TEST_FP_OP2_S(62009,  fmul.s, 1,      3.14159265e-8, 3.14159265, 0.00000001 );

  # Is the canonical NaN generated for Inf - Inf?
  TEST_FP_OP2_S_HEX(62010,  fsub.s, 0x10, qNaNf, Inff, Inff);

  TEST_PASSFAIL
FUNC_EPILOGUE


# ======================================================================
# rv32uf: fdiv
# ======================================================================
FUNC_PROLOGUE test_rv32uf_fdiv

  #-------------------------------------------------------------
  # Arithmetic tests
  #-------------------------------------------------------------

  TEST_FP_OP2_S(63001,  fdiv.s, 1, 1.1557273520668288, 3.14159265, 2.71828182 );
  TEST_FP_OP2_S(63002,  fdiv.s, 1,-0.9991093838555584,      -1234,     1235.1 );
  TEST_FP_OP2_S(63003,  fdiv.s, 0,         3.14159265, 3.14159265,        1.0 );

  TEST_FP_OP1_S(63004,  fsqrt.s, 1, 1.7724538498928541, 3.14159265 );
  TEST_FP_OP1_S(63005,  fsqrt.s, 0,                100,      10000 );

  TEST_FP_OP1_S_DWORD_RESULT(63006,  fsqrt.s, 0x10, 0x7FC00000, -1.0 );

  TEST_FP_OP1_S(63007,  fsqrt.s, 1, 13.076696, 171.0);

  TEST_PASSFAIL
FUNC_EPILOGUE


# ======================================================================
# rv32uf: fclass
# ======================================================================
FUNC_PROLOGUE test_rv32uf_fclass

  #-------------------------------------------------------------
  # Arithmetic tests
  #-------------------------------------------------------------

  TEST_FCLASS_S(64001, 1 << 0, nInff )
  TEST_FCLASS_S(64002, 1 << 1, 0xbf800000 )
  TEST_FCLASS_S(64003, 1 << 2, 0x807fffff )
  TEST_FCLASS_S(64004, 1 << 3, 0x80000000 )
  TEST_FCLASS_S(64005, 1 << 4, 0x00000000 )
  TEST_FCLASS_S(64006, 1 << 5, 0x007fffff )
  TEST_FCLASS_S(64007, 1 << 6, 0x3f800000 )
  TEST_FCLASS_S(64008, 1 << 7, Inff )
  TEST_FCLASS_S(64009, 1 << 8, sNaNf )
  TEST_FCLASS_S(64010, 1 << 9, qNaNf )

  TEST_PASSFAIL
FUNC_EPILOGUE


# ======================================================================
# rv32uf: fcmp
# ======================================================================
FUNC_PROLOGUE test_rv32uf_fcmp

  #-------------------------------------------------------------
  # Arithmetic tests
  #-------------------------------------------------------------

  TEST_FP_CMP_OP_S(65001, feq.s, 0x00, 1, -1.36, -1.36)
  TEST_FP_CMP_OP_S(65002, fle.s, 0x00, 1, -1.36, -1.36)
  TEST_FP_CMP_OP_S(65003, flt.s, 0x00, 0, -1.36, -1.36)

  TEST_FP_CMP_OP_S(65004, feq.s, 0x00, 0, -1.37, -1.36)
  TEST_FP_CMP_OP_S(65005, fle.s, 0x00, 1, -1.37, -1.36)
  TEST_FP_CMP_OP_S(65006, flt.s, 0x00, 1, -1.37, -1.36)

  # Only sNaN should signal invalid for feq.
  TEST_FP_CMP_OP_S(65007, feq.s, 0x00, 0, NaN, 0)
  TEST_FP_CMP_OP_S(65008, feq.s, 0x00, 0, NaN, NaN)
  TEST_FP_CMP_OP_S_HEX(65009, feq.s, 0x10, 0, sNaNf, 0)

  # qNaN should signal invalid for fle/flt.
  TEST_FP_CMP_OP_S(65010, flt.s, 0x10, 0, NaN, 0)
  TEST_FP_CMP_OP_S(65011, flt.s, 0x10, 0, NaN, NaN)
  TEST_FP_CMP_OP_S_HEX(65012, flt.s, 0x10, 0, sNaNf, 0)
  TEST_FP_CMP_OP_S(65013, fle.s, 0x10, 0, NaN, 0)
  TEST_FP_CMP_OP_S(65014, fle.s, 0x10, 0, NaN, NaN)
  TEST_FP_CMP_OP_S_HEX(65015, fle.s, 0x10, 0, sNaNf, 0)

  TEST_PASSFAIL
FUNC_EPILOGUE


# ======================================================================
# rv32uf: fcvt
# ======================================================================
FUNC_PROLOGUE test_rv32uf_fcvt

  #-------------------------------------------------------------
  # Arithmetic tests
  #-------------------------------------------------------------

  TEST_INT_FP_OP_S(66001,  fcvt.s.w,                   2.0,  2);
  TEST_INT_FP_OP_S(66002,  fcvt.s.w,                  -2.0, -2);

  TEST_INT_FP_OP_S(66003, fcvt.s.wu,                   2.0,  2);
  TEST_INT_FP_OP_S(66004, fcvt.s.wu,           4.2949673e9, -2);

#if __riscv_xlen >= 64
  TEST_INT_FP_OP_S(66005,  fcvt.s.l,                   2.0,  2);
  TEST_INT_FP_OP_S(66006,  fcvt.s.l,                  -2.0, -2);

  TEST_INT_FP_OP_S(66007, fcvt.s.lu,                   2.0,  2);
  TEST_INT_FP_OP_S(66008, fcvt.s.lu,          1.8446744e19, -2);
#endif

  TEST_PASSFAIL
FUNC_EPILOGUE


# ======================================================================
# rv32uf: fmadd
# ======================================================================
FUNC_PROLOGUE test_rv32uf_fmadd

  #-------------------------------------------------------------
  # Arithmetic tests
  #-------------------------------------------------------------

  TEST_FP_OP3_S(67001,  fmadd.s, 0,                 3.5,  1.0,        2.5,        1.0 );
  TEST_FP_OP3_S(67002,  fmadd.s, 1,              1236.2, -1.0,    -1235.1,        1.1 );
  TEST_FP_OP3_S(67003,  fmadd.s, 0,               -12.0,  2.0,       -5.0,       -2.0 );

  TEST_FP_OP3_S(67004, fnmadd.s, 0,                -3.5,  1.0,        2.5,        1.0 );
  TEST_FP_OP3_S(67005, fnmadd.s, 1,             -1236.2, -1.0,    -1235.1,        1.1 );
  TEST_FP_OP3_S(67006, fnmadd.s, 0,                12.0,  2.0,       -5.0,       -2.0 );

  TEST_FP_OP3_S(67007,  fmsub.s, 0,                 1.5,  1.0,        2.5,        1.0 );
  TEST_FP_OP3_S(67008,  fmsub.s, 1,                1234, -1.0,    -1235.1,        1.1 );
  TEST_FP_OP3_S(67009,  fmsub.s, 0,                -8.0,  2.0,       -5.0,       -2.0 );

  TEST_FP_OP3_S(67010, fnmsub.s, 0,                -1.5,  1.0,        2.5,        1.0 );
  TEST_FP_OP3_S(67011, fnmsub.s, 1,               -1234, -1.0,    -1235.1,        1.1 );
  TEST_FP_OP3_S(67012, fnmsub.s, 0,                 8.0,  2.0,       -5.0,       -2.0 );

  TEST_PASSFAIL
FUNC_EPILOGUE


# ======================================================================
# rv32uf: fmin
# ======================================================================
FUNC_PROLOGUE test_rv32uf_fmin

  #-------------------------------------------------------------
  # Arithmetic tests
  #-------------------------------------------------------------

  TEST_FP_OP2_S(68001,  fmin.s, 0,        1.0,        2.5,        1.0 );
  TEST_FP_OP2_S(68002,  fmin.s, 0,    -1235.1,    -1235.1,        1.1 );
  TEST_FP_OP2_S(68003,  fmin.s, 0,    -1235.1,        1.1,    -1235.1 );
  TEST_FP_OP2_S(68004,  fmin.s, 0,    -1235.1,        NaN,    -1235.1 );
  TEST_FP_OP2_S(68005,  fmin.s, 0, 0.00000001, 3.14159265, 0.00000001 );
  TEST_FP_OP2_S(68006,  fmin.s, 0,       -2.0,       -1.0,       -2.0 );

  TEST_FP_OP2_S(68007,  fmax.s, 0,        2.5,        2.5,        1.0 );
  TEST_FP_OP2_S(68008,  fmax.s, 0,        1.1,    -1235.1,        1.1 );
  TEST_FP_OP2_S(68009,  fmax.s, 0,        1.1,        1.1,    -1235.1 );
  TEST_FP_OP2_S(68010,  fmax.s, 0,    -1235.1,        NaN,    -1235.1 );
  TEST_FP_OP2_S(68011,  fmax.s, 0, 3.14159265, 3.14159265, 0.00000001 );
  TEST_FP_OP2_S(68012,  fmax.s, 0,       -1.0,       -1.0,       -2.0 );

  # FMAX(sNaN, x) = x
  TEST_FP_OP2_S_HEX(68013,  fmax.s, 0x10, 0x3f800000, sNaNf, 0x3f800000);
  # FMAX(qNaN, qNaN) = canonical NaN
  TEST_FP_OP2_S_HEX(68014,  fmax.s, 0x00, qNaNf, 0x7fffffff, 0x7fffffff);

  # -0.0 < +0.0
  TEST_FP_OP2_S(68015,  fmin.s, 0,       -0.0,       -0.0,        0.0 );
  TEST_FP_OP2_S(68016,  fmin.s, 0,       -0.0,        0.0,       -0.0 );
  TEST_FP_OP2_S(68017,  fmax.s, 0,        0.0,       -0.0,        0.0 );
  TEST_FP_OP2_S(68018,  fmax.s, 0,        0.0,        0.0,       -0.0 );

  TEST_PASSFAIL
FUNC_EPILOGUE


# ======================================================================
# rv32uf: fcvt_w
# ======================================================================
FUNC_PROLOGUE test_rv32uf_fcvt_w

  #-------------------------------------------------------------
  # Arithmetic tests (rv32)
  #-------------------------------------------------------------

  TEST_FP_INT_OP_S(69001,  fcvt.w.s, 0x01,         -1, -1.1, rtz);
  TEST_FP_INT_OP_S(69002,  fcvt.w.s, 0x00,         -1, -1.0, rtz);
  TEST_FP_INT_OP_S(69003,  fcvt.w.s, 0x01,          0, -0.9, rtz);
  TEST_FP_INT_OP_S(69004,  fcvt.w.s, 0x01,          0,  0.9, rtz);
  TEST_FP_INT_OP_S(69005,  fcvt.w.s, 0x00,          1,  1.0, rtz);
  TEST_FP_INT_OP_S(69006,  fcvt.w.s, 0x01,          1,  1.1, rtz);
  TEST_FP_INT_OP_S(69007,  fcvt.w.s, 0x10,     -1<<31, -3e9, rtz);
  TEST_FP_INT_OP_S(69008,  fcvt.w.s, 0x10,  (1<<31)-1,  3e9, rtz);

  TEST_FP_INT_OP_S(69009, fcvt.wu.s, 0x10,          0, -3.0, rtz);
  TEST_FP_INT_OP_S(69010, fcvt.wu.s, 0x10,          0, -1.0, rtz);
  TEST_FP_INT_OP_S(69011, fcvt.wu.s, 0x01,          0, -0.9, rtz);
  TEST_FP_INT_OP_S(69012, fcvt.wu.s, 0x01,          0,  0.9, rtz);
  TEST_FP_INT_OP_S(69013, fcvt.wu.s, 0x00,          1,  1.0, rtz);
  TEST_FP_INT_OP_S(69014, fcvt.wu.s, 0x01,          1,  1.1, rtz);
  TEST_FP_INT_OP_S(69015, fcvt.wu.s, 0x10,          0, -3e9, rtz);
  TEST_FP_INT_OP_S(69016, fcvt.wu.s, 0x00, 3000000000,  3e9, rtz);

  # test negative NaN, negative infinity conversion (rv32)
  TEST_CASE(69017, x1, 0x000000007fffffff, la x1, tdat_fcvt_w  ; flw f1,  0(x1); fcvt.w.s x1, f1)
  TEST_CASE(69018, x1, 0xffffffff80000000, la x1, tdat_fcvt_w  ; flw f1,  8(x1); fcvt.w.s x1, f1)

  # test positive NaN, positive infinity conversion (rv32)
  TEST_CASE(69019, x1, 0x000000007fffffff, la x1, tdat_fcvt_w  ; flw f1,  4(x1); fcvt.w.s x1, f1)
  TEST_CASE(69020, x1, 0x000000007fffffff, la x1, tdat_fcvt_w  ; flw f1, 12(x1); fcvt.w.s x1, f1)

  # test NaN, infinity conversions to unsigned integer (rv32)
  TEST_CASE(69021, x1, 0xffffffffffffffff, la x1, tdat_fcvt_w  ; flw f1,  0(x1); fcvt.wu.s x1, f1)
  TEST_CASE(69022, x1, 0xffffffffffffffff, la x1, tdat_fcvt_w  ; flw f1,  4(x1); fcvt.wu.s x1, f1)
  TEST_CASE(69023, x1,                  0, la x1, tdat_fcvt_w  ; flw f1,  8(x1); fcvt.wu.s x1, f1)
  TEST_CASE(69024, x1, 0xffffffffffffffff, la x1, tdat_fcvt_w  ; flw f1, 12(x1); fcvt.wu.s x1, f1)

#if __riscv_xlen >= 64
  #-------------------------------------------------------------
  # Arithmetic tests (rv64)
  #-------------------------------------------------------------

  TEST_FP_INT_OP_S(69025,  fcvt.l.s, 0x01,         -1, -1.1, rtz);
  TEST_FP_INT_OP_S(69026,  fcvt.l.s, 0x00,         -1, -1.0, rtz);
  TEST_FP_INT_OP_S(69027,  fcvt.l.s, 0x01,          0, -0.9, rtz);
  TEST_FP_INT_OP_S(69028,  fcvt.l.s, 0x01,          0,  0.9, rtz);
  TEST_FP_INT_OP_S(69029,  fcvt.l.s, 0x00,          1,  1.0, rtz);
  TEST_FP_INT_OP_S(69030,  fcvt.l.s, 0x01,          1,  1.1, rtz);

  TEST_FP_INT_OP_S(69031, fcvt.lu.s, 0x10,          0, -3.0, rtz);
  TEST_FP_INT_OP_S(69032, fcvt.lu.s, 0x10,          0, -1.0, rtz);
  TEST_FP_INT_OP_S(69033, fcvt.lu.s, 0x01,          0, -0.9, rtz);
  TEST_FP_INT_OP_S(69034, fcvt.lu.s, 0x01,          0,  0.9, rtz);
  TEST_FP_INT_OP_S(69035, fcvt.lu.s, 0x00,          1,  1.0, rtz);
  TEST_FP_INT_OP_S(69036, fcvt.lu.s, 0x01,          1,  1.1, rtz);
  TEST_FP_INT_OP_S(69037, fcvt.lu.s, 0x10,          0, -3e9, rtz);

  # test negative NaN, negative infinity conversion (rv64)
  TEST_CASE(69038, x1, 0x7fffffffffffffff, la x1, tdat_fcvt_w  ; flw f1,  0(x1); fcvt.l.s x1, f1)
  TEST_CASE(69039, x1, 0x8000000000000000, la x1, tdat_fcvt_w  ; flw f1,  8(x1); fcvt.l.s x1, f1)

  # test positive NaN, positive infinity conversion (rv64)
  TEST_CASE(69040, x1, 0x7fffffffffffffff, la x1, tdat_fcvt_w  ; flw f1,  4(x1); fcvt.l.s x1, f1)
  TEST_CASE(69041, x1, 0x7fffffffffffffff, la x1, tdat_fcvt_w  ; flw f1, 12(x1); fcvt.l.s x1, f1)

  # test NaN, infinity conversions to unsigned integer (rv64)
  TEST_CASE(69042, x1, 0xffffffffffffffff, la x1, tdat_fcvt_w  ; flw f1,  0(x1); fcvt.lu.s x1, f1)
  TEST_CASE(69043, x1, 0xffffffffffffffff, la x1, tdat_fcvt_w  ; flw f1,  4(x1); fcvt.lu.s x1, f1)
  TEST_CASE(69044, x1,                  0, la x1, tdat_fcvt_w  ; flw f1,  8(x1); fcvt.lu.s x1, f1)
  TEST_CASE(69045, x1, 0xffffffffffffffff, la x1, tdat_fcvt_w  ; flw f1, 12(x1); fcvt.lu.s x1, f1)
#endif

  TEST_PASSFAIL
FUNC_EPILOGUE


# ======================================================================
# rv32uf: ldst
# ======================================================================
FUNC_PROLOGUE test_rv32uf_ldst

#if __riscv_xlen >= 64
  TEST_CASE(70001, a0, 0x40000000deadbeef, la a1, tdat_fldst; flw f1, 4(a1); fsw f1, 20(a1); ld a0, 16(a1))
  TEST_CASE(70002, a0, 0x1337d00dbf800000, la a1, tdat_fldst; flw f1, 0(a1); fsw f1, 24(a1); ld a0, 24(a1))
#endif

  li TESTNUM,70
  TEST_PASSFAIL
FUNC_EPILOGUE


# ======================================================================
# rv32uf: move
# ======================================================================
FUNC_PROLOGUE test_rv32uf_move

  TEST_CASE(71001, a1, 1, csrwi fcsr, 1; li a0, 0x1234; fssr a1, a0)
  TEST_CASE(71002, a0, 0x34, frsr a0)
  TEST_CASE(71003, a0, 0x14, frflags a0)
  TEST_CASE(71004, a0, 0x01, csrrwi a0, frm, 2)
  TEST_CASE(71005, a0, 0x54, frsr a0)
  TEST_CASE(71006, a0, 0x14, csrrci a0, fflags, 4)
  TEST_CASE(71007, a0, 0x50, frsr a0)

#define TEST_FSGNJS(n, insn, new_sign, rs1_sign, rs2_sign) \
  TEST_CASE(n, a0, 0x12345678 | (-(new_sign) << 31), \
    li a1, ((rs1_sign) << 31) | 0x12345678; \
    li a2, -(rs2_sign); \
    fmv.s.x f1, a1; \
    fmv.s.x f2, a2; \
    insn f0, f1, f2; \
    fmv.x.s a0, f0)

  TEST_FSGNJS(71008, fsgnj.s, 0, 0, 0)
  TEST_FSGNJS(71009, fsgnj.s, 1, 0, 1)
  TEST_FSGNJS(71010, fsgnj.s, 0, 1, 0)
  TEST_FSGNJS(71011, fsgnj.s, 1, 1, 1)

  TEST_FSGNJS(71012, fsgnjn.s, 1, 0, 0)
  TEST_FSGNJS(71013, fsgnjn.s, 0, 0, 1)
  TEST_FSGNJS(71014, fsgnjn.s, 1, 1, 0)
  TEST_FSGNJS(71015, fsgnjn.s, 0, 1, 1)

  TEST_FSGNJS(71016, fsgnjx.s, 0, 0, 0)
  TEST_FSGNJS(71017, fsgnjx.s, 1, 0, 1)
  TEST_FSGNJS(71018, fsgnjx.s, 1, 1, 0)
  TEST_FSGNJS(71019, fsgnjx.s, 0, 1, 1)

  TEST_PASSFAIL
FUNC_EPILOGUE


# ======================================================================
# rv32uf: recoding
# ======================================================================
FUNC_PROLOGUE test_rv32uf_recoding

  # Make sure infinities with different mantissas compare as equal.
  flw f0, minf, a0
  flw f1, three, a0
  fmul.s f1, f1, f0
  TEST_CASE(72001, a0, 1, feq.s a0, f0, f1)
  TEST_CASE(72002, a0, 1, fle.s a0, f0, f1)
  TEST_CASE(72003, a0, 0, flt.s a0, f0, f1)

  # Likewise, but for zeroes.
  fcvt.s.w f0, x0
  li a0, 1
  fcvt.s.w f1, a0
  fmul.s f1, f1, f0
  TEST_CASE(72004, a0, 1, feq.s a0, f0, f1)
  TEST_CASE(72005, a0, 1, fle.s a0, f0, f1)
  TEST_CASE(72006, a0, 0, flt.s a0, f0, f1)

  TEST_PASSFAIL
FUNC_EPILOGUE

# ======================================================================
# Data Sections
# ======================================================================
.section .data
.align 3
_test_ctx_ra: .word 0
_test_ctx_sp: .word 0

# Load/store test data (used by lb, lbu, lh, lhu, lw, sb, sh, sw, or, ori, etc.)
tdat:
tdat1:  .word 0x00ff00ff
tdat2:  .word 0xff00ff00
tdat3:  .word 0x0ff00ff0
tdat4:  .word 0xf00ff00f
tdat5:  .word 0x00000000
tdat6:  .word 0xffffffff
tdat7:  .word 0xfffff0f0
tdat8:  .word 0x0000f0f0
tdat9:  .word 0xdeadbeef
tdat10: .word 0xdeadbeef


data_rvc:
data_rvc1:          .dword 0xfedcba9876543210
data_rvc2:          .dword 0xfedcba9876543210
    .align 12
    .skip 4094

# ld_st test data
tdat_ldst:
    .rept 20
    .word 0xdeadbeef
    .endr

# st_ld test data
tdat_stld:
    .rept 20
    .word 0xdeadbeef
    .endr

# fence_i data
insn_fence_i:
  addi a3, a3, 333
2: addi a3, a3, 222
   jalr a5, t1, 0
3: addi a3, a3, 555
   jalr a5, t1, 0

# ma_data data
data_ma:
  .align 3
  .word 0x03020100
  .word 0x07060504
  .word 0x0b0a0908
  .word 0x0f0e0d0c
  .word 0x13121110
  .word 0x17161514
  .word 0x1b1a1918
  .word 0x1f1e1d1c
  .word 0x23222120
  .word 0x27262524
  .word 0x2b2a2928
  .word 0x2f2e2d2c
  .word 0x33323130
  .word 0x37363534
  .word 0x3b3a3938
  .word 0x3f3e3d3c
  .word 0x43424140
  .word 0x47464544
  .word 0x4b4a4948
  .word 0x4f4e4d4c
  .word 0x53525150
  .word 0x57565554
  .word 0x5b5a5958
  .word 0x5f5e5d5c
  .word 0x63626160
  .word 0x67666564
  .word 0x6b6a6968
  .word 0x6f6e6d6c
  .word 0x73727170
  .word 0x77767574
  .word 0x7b7a7978
  .word 0x7f7e7d7c
  .fill 0xff, 1, 80

# lrsc data
coreid_lrsc: .word 0
barrier_lrsc: .word 0
foo_lrsc: .word 0
.skip 1024
fooTest3_lrsc: .word 0

# FP test data
.align 3
tdat_fcvt_w:
.word 0xffffffff
.word 0x7fffffff
.word nInff
.word Inff

tdat_fldst:
.word 0xbf800000
.word 0x40000000
.word 0x40400000
.word 0xc0800000
.word 0xdeadbeef
.word 0xcafebabe
.word 0xabad1dea
.word 0x1337d00d

minf: .float -Inf
three: .float 3.0

# ======================================================================
# BSS Section
# ======================================================================
.section .bss
.align 3
amo_operand_amoadd_w:  .dword 0
amo_operand_amoand_w:  .dword 0
amo_operand_amomax_w:  .dword 0
amo_operand_amomaxu_w: .dword 0
amo_operand_amomin_w:  .dword 0
amo_operand_amominu_w: .dword 0
amo_operand_amoor_w:   .dword 0
amo_operand_amoxor_w:  .dword 0
amo_operand_amoswap_w: .dword 0

# ======================================================================
# Stack Section
# ======================================================================
.section .stack
.align 4
_stack_bottom:
  .space 4096
_stack_top:
