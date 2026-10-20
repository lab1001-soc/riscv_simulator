/*
 * test_isa.c - RV32 IMACF ISA Test Dispatcher
 *
 * Calls each test function and reports pass/fail.
 * Returns 0 if all tests pass, non-zero count of failures otherwise.
 *
 * Each test function returns:
 *   0              = pass
 *   (testnum<<1)|1 = fail, where testnum is the failing test case number
 */

/* ---- rv32ui: Base Integer (38 tests) ---- */
extern int test_rv32ui_add(void);
extern int test_rv32ui_addi(void);
extern int test_rv32ui_and(void);
extern int test_rv32ui_andi(void);
extern int test_rv32ui_auipc(void);
extern int test_rv32ui_beq(void);
extern int test_rv32ui_bge(void);
extern int test_rv32ui_bgeu(void);
extern int test_rv32ui_blt(void);
extern int test_rv32ui_bltu(void);
extern int test_rv32ui_bne(void);
extern int test_rv32ui_jal(void);
extern int test_rv32ui_jalr(void);
extern int test_rv32ui_lb(void);
extern int test_rv32ui_lbu(void);
extern int test_rv32ui_lh(void);
extern int test_rv32ui_lhu(void);
extern int test_rv32ui_lui(void);
extern int test_rv32ui_lw(void);
extern int test_rv32ui_or(void);
extern int test_rv32ui_ori(void);
extern int test_rv32ui_sb(void);
extern int test_rv32ui_sh(void);
extern int test_rv32ui_simple(void);
extern int test_rv32ui_sll(void);
extern int test_rv32ui_slli(void);
extern int test_rv32ui_slt(void);
extern int test_rv32ui_slti(void);
extern int test_rv32ui_sltiu(void);
extern int test_rv32ui_sltu(void);
extern int test_rv32ui_sra(void);
extern int test_rv32ui_srai(void);
extern int test_rv32ui_srl(void);
extern int test_rv32ui_srli(void);
extern int test_rv32ui_sub(void);
extern int test_rv32ui_sw(void);
extern int test_rv32ui_xor(void);
extern int test_rv32ui_xori(void);
extern int test_rv32ui_ld_st(void);
extern int test_rv32ui_st_ld(void);
extern int test_rv32ui_fence_i(void);
extern int test_rv32ui_ma_data(void);

/* ---- rv32um: Multiply/Divide (8 tests) ---- */
extern int test_rv32um_mul(void);
extern int test_rv32um_mulh(void);
extern int test_rv32um_mulhsu(void);
extern int test_rv32um_mulhu(void);
extern int test_rv32um_div(void);
extern int test_rv32um_divu(void);
extern int test_rv32um_rem(void);
extern int test_rv32um_remu(void);

/* ---- rv32ua: Atomic (10 tests) ---- */
extern int test_rv32ua_amoadd_w(void);
extern int test_rv32ua_amoand_w(void);
extern int test_rv32ua_amomax_w(void);
extern int test_rv32ua_amomaxu_w(void);
extern int test_rv32ua_amomin_w(void);
extern int test_rv32ua_amominu_w(void);
extern int test_rv32ua_amoor_w(void);
extern int test_rv32ua_amoxor_w(void);
extern int test_rv32ua_amoswap_w(void);
extern int test_rv32ua_lrsc(void);

/* ---- rv32uc: Compressed (1 test) ---- */
extern int test_rv32uc_rvc(void);

/* ---- rv32uf: Single-Precision Float (11 tests) ---- */
extern int test_rv32uf_fadd(void);
extern int test_rv32uf_fdiv(void);
extern int test_rv32uf_fclass(void);
extern int test_rv32uf_fcmp(void);
extern int test_rv32uf_fcvt(void);
extern int test_rv32uf_fcvt_w(void);
extern int test_rv32uf_fmadd(void);
extern int test_rv32uf_fmin(void);
extern int test_rv32uf_ldst(void);
extern int test_rv32uf_move(void);
extern int test_rv32uf_recoding(void);
#include <string.h>
#include "MyPrintf.h"
/* ---- Test table ---- */
typedef struct {
    const char *name;
    int (*func)(void);
} test_entry_t;

static const test_entry_t all_tests[] = {
    /* rv32ui - Base Integer */
    { "rv32ui-add",       test_rv32ui_add },
    { "rv32ui-addi",      test_rv32ui_addi },
    { "rv32ui-and",       test_rv32ui_and },
    { "rv32ui-andi",      test_rv32ui_andi },
    { "rv32ui-auipc",     test_rv32ui_auipc },
    { "rv32ui-beq",       test_rv32ui_beq },
    { "rv32ui-bge",       test_rv32ui_bge },
    { "rv32ui-bgeu",      test_rv32ui_bgeu },
    { "rv32ui-blt",       test_rv32ui_blt },
    { "rv32ui-bltu",      test_rv32ui_bltu },
    { "rv32ui-bne",       test_rv32ui_bne },
    { "rv32ui-jal",       test_rv32ui_jal },
    { "rv32ui-jalr",      test_rv32ui_jalr },
    { "rv32ui-lb",        test_rv32ui_lb },
    { "rv32ui-lbu",       test_rv32ui_lbu },
    { "rv32ui-lh",        test_rv32ui_lh },
    { "rv32ui-lhu",       test_rv32ui_lhu },
    { "rv32ui-lui",       test_rv32ui_lui },
    { "rv32ui-lw",        test_rv32ui_lw },
    { "rv32ui-or",        test_rv32ui_or },
    { "rv32ui-ori",       test_rv32ui_ori },
    { "rv32ui-sb",        test_rv32ui_sb },
    { "rv32ui-sh",        test_rv32ui_sh },
    // { "rv32ui-simple",    test_rv32ui_simple },
    { "rv32ui-sll",       test_rv32ui_sll },
    { "rv32ui-slli",      test_rv32ui_slli },
    { "rv32ui-slt",       test_rv32ui_slt },
    { "rv32ui-slti",      test_rv32ui_slti },
    { "rv32ui-sltiu",     test_rv32ui_sltiu },
    { "rv32ui-sltu",      test_rv32ui_sltu },
    { "rv32ui-sra",       test_rv32ui_sra },
    { "rv32ui-srai",      test_rv32ui_srai },
    { "rv32ui-srl",       test_rv32ui_srl },
    { "rv32ui-srli",      test_rv32ui_srli },
    { "rv32ui-sub",       test_rv32ui_sub },
    { "rv32ui-sw",        test_rv32ui_sw },
    { "rv32ui-xor",       test_rv32ui_xor },
    { "rv32ui-xori",      test_rv32ui_xori },
    { "rv32ui-ld_st",     test_rv32ui_ld_st },
    { "rv32ui-st_ld",     test_rv32ui_st_ld },
    // { "rv32ui-fence_i",   test_rv32ui_fence_i },//TODO: 修改flash 数据，待修改
    // { "rv32ui-ma_data",   test_rv32ui_ma_data }, //TODO: 该用例依赖非对齐访问陷阱，模拟器允许非对齐访问，待实现异常后再启用

    /* rv32um - Multiply/Divide */
    { "rv32um-mul",       test_rv32um_mul },
    { "rv32um-mulh",      test_rv32um_mulh },
    { "rv32um-mulhsu",    test_rv32um_mulhsu },
    { "rv32um-mulhu",     test_rv32um_mulhu },
    { "rv32um-div",       test_rv32um_div },
    { "rv32um-divu",      test_rv32um_divu },
    { "rv32um-rem",       test_rv32um_rem },
    { "rv32um-remu",      test_rv32um_remu },

    /* rv32ua - Atomic */
    { "rv32ua-amoadd_w",  test_rv32ua_amoadd_w },
    { "rv32ua-amoand_w",  test_rv32ua_amoand_w },
    { "rv32ua-amomax_w",  test_rv32ua_amomax_w },
    { "rv32ua-amomaxu_w", test_rv32ua_amomaxu_w },
    { "rv32ua-amomin_w",  test_rv32ua_amomin_w },
    { "rv32ua-amominu_w", test_rv32ua_amominu_w },
    { "rv32ua-amoor_w",   test_rv32ua_amoor_w },
    { "rv32ua-amoxor_w",  test_rv32ua_amoxor_w },
    { "rv32ua-amoswap_w", test_rv32ua_amoswap_w },
    // { "rv32ua-lrsc",      test_rv32ua_lrsc }, //TODO: 多核才会使用，当前会无限循环，待修改

    /* rv32uc - Compressed */
    { "rv32uc-rvc",       test_rv32uc_rvc },

    /* rv32uf - Single-Precision Float */
    { "rv32uf-fadd",      test_rv32uf_fadd },
    { "rv32uf-fdiv",      test_rv32uf_fdiv },
    { "rv32uf-fclass",    test_rv32uf_fclass },
    { "rv32uf-fcmp",      test_rv32uf_fcmp },
    { "rv32uf-fcvt",      test_rv32uf_fcvt },
    { "rv32uf-fcvt_w",    test_rv32uf_fcvt_w },
    { "rv32uf-fmadd",     test_rv32uf_fmadd },
    { "rv32uf-fmin",      test_rv32uf_fmin },
    { "rv32uf-ldst",      test_rv32uf_ldst },
    { "rv32uf-move",      test_rv32uf_move },
    { "rv32uf-recoding",  test_rv32uf_recoding },

    { 0, 0 }
};

/*
 * test_isa - Run all IMACF ISA compliance tests.
 *
 * Returns: 0 if all tests pass, or the number of failed tests.
 *
 * To get details on failures, examine the return value of each test function:
 *   result == 0       -> pass
 *   result & 1         -> 1 indicates failure
 *   result >> 1        -> failing test case number
 */
int test_isa(void)
{
    int total = 0, passed = 0, failed = 0;
    const test_entry_t *t;
    MyPrintf("test_isa\r\n");

    for (t = all_tests; t->name; t++) {
        int result = t->func();
        total++;
        if (result == 0) {
            passed++;
        } else {
            failed++;
            /* result >> 1 = failing test number */
            /* result & 1  = 1 (failure indicator) */
        }
        int nlen = strlen(t->name);
        MyPrintf("test[%02d] %s", total, t->name);
        for (int i = 0; i < 20-nlen; i++)
            MyPrintf(" ");
        MyPrintf("%s %d\r\n", !(result & 1) ? "pass" : "fail", result >> 1);
    }

    return failed;
}
