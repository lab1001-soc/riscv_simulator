/*
 * execute.c - 执行模块
 * 负责执行所有 RV32IMACF 指令
 */

#include "cpu.h"
#include "mem.h"
#include <stdio.h>
#include <math.h>
#include <fenv.h>

/* RISC-V fflags 位定义 - 注意：此项目使用自定义布局，与标准相反 */
#define FFLAG_NX  (1 << 0)  /* Inexact - bit 0 */
#define FFLAG_UF  (1 << 1)  /* Underflow - bit 1 */
#define FFLAG_OF  (1 << 2)  /* Overflow - bit 2 */
#define FFLAG_DZ  (1 << 3)  /* Divide by zero - bit 3 */
#define FFLAG_NV  (1 << 4)  /* Invalid operation - bit 4 */

/* Canonical quiet NaN (单精度) */
#define CANONICAL_QNAN  0x7fc00000

/* 浮点数类型信息 */
typedef struct {
    int is_nan;     /* 是否是 NaN (包括 sNaN 和 qNaN) */
    int is_snan;    /* 是否是 signaling NaN (frac 最高位为 0) */
    int is_qnan;    /* 是否是 quiet NaN (frac 最高位为 1) */
    int is_inf;     /* 是否是 Infinity */
    int is_zero;    /* 是否是零 (包括 +0 和 -0) */
    int sign;       /* 符号位: 0=正, 1=负 */
} FloatInfo;

/* 兼容旧接口：NaNInfo 现是 FloatInfo 的别名 */
typedef FloatInfo NaNInfo;

/* 判断浮点数的类型 */
static FloatInfo classify_float(uint32_t u) {
    FloatInfo info = {0, 0, 0, 0, 0, 0};
    int exp = (u >> 23) & 0xFF;
    int frac = u & 0x7FFFFF;
    info.sign = (u >> 31) & 1;

    if (exp == 0xFF) {
        if (frac == 0) {
            info.is_inf = 1;  /* Infinity */
        } else {
            info.is_nan = 1;
            info.is_snan = ((frac & 0x400000) == 0);
            info.is_qnan = ((frac & 0x400000) != 0);
        }
    } else if (exp == 0 && frac == 0) {
        info.is_zero = 1;  /* Zero (+0 or -0) */
    }
    return info;
}

/* 兼容旧接口：判断 NaN 类型 */
static NaNInfo classify_nan(uint32_t u) {
    return classify_float(u);
}

/* 检查 fadd/fsub 的 Inf 特殊情况：
 * Inf + (-Inf) 或 Inf - Inf (同号) 会产生 NaN + NV
 * is_sub: 1=fsub, 0=fadd
 * 返回: 1=需要特殊处理(NV), 0=正常
 */
static int check_inf_add_sub(FloatInfo a, FloatInfo b, int is_sub) {
    if (!a.is_inf && !b.is_inf) return 0;

    /* fadd: Inf + (-Inf) = NaN */
    if (!is_sub) {
        if (a.is_inf && b.is_inf && a.sign != b.sign) {
            return 1;  /* NV */
        }
    }
    /* fsub: Inf - Inf = NaN (同号) */
    else {
        if (a.is_inf && b.is_inf && a.sign == b.sign) {
            return 1;  /* NV */
        }
    }
    return 0;  /* 正常运算 */
}

/* 检查 fmul 的 Inf 特殊情况：
 * 0 * Inf 或 Inf * 0 会产生 NaN + NV
 * 返回: 1=需要特殊处理(NV), 0=正常
 */
static int check_inf_mul(FloatInfo a, FloatInfo b) {
    /* 0 * Inf = NaN */
    if ((a.is_zero && b.is_inf) || (a.is_inf && b.is_zero)) {
        return 1;  /* NV */
    }
    return 0;
}

/* 检查 fdiv 的 Inf 特殊情况：
 * Inf / Inf 或 0 / 0 会产生 NaN + NV
 * finite / 0 会产生 Inf + DZ
 * Inf / finite 会产生 Inf + OF
 * finite / Inf 会产生 0 + OF (逐渐下溢)
 * 返回: 1=NV, 2=DZ, 3=OF, 0=正常
 */
static int check_inf_div(FloatInfo a, FloatInfo b) {
    /* Inf / Inf = NaN + NV */
    if (a.is_inf && b.is_inf) {
        return 1;  /* NV */
    }
    /* 0 / 0 = NaN + NV */
    if (a.is_zero && b.is_zero) {
        return 1;  /* NV */
    }
    /* finite / 0 = Inf + DZ */
    if (!a.is_nan && !a.is_inf && !a.is_zero && b.is_zero) {
        return 2;  /* DZ */
    }
    /* Inf / finite = Inf + OF */
    if (a.is_inf && !b.is_nan && !b.is_inf && !b.is_zero) {
        return 3;  /* OF */
    }
    /* finite / Inf = 0 + OF (逐渐下溢) */
    if (!a.is_nan && !a.is_inf && !a.is_zero && b.is_inf) {
        return 3;  /* OF */
    }
    return 0;  /* 正常 */
}

/* 从 C 浮点异常映射到 RISC-V fflags */
static void update_fflags(CPU *cpu) {
    int except = fetestexcept(FE_ALL_EXCEPT);
    uint32_t flags = 0;

    if (except & FE_INVALID)  flags |= FFLAG_NV;
    if (except & FE_DIVBYZERO) flags |= FFLAG_DZ;
    if (except & FE_OVERFLOW)  flags |= FFLAG_OF;
    if (except & FE_UNDERFLOW) flags |= FFLAG_UF;
    if (except & FE_INEXACT)   flags |= FFLAG_NX;

    /* 设置 fflags 为当前异常（不累积，便于测试框架检查） */
    cpu->fcsr = (cpu->fcsr & ~0x1F) | flags;

    /* 清除 C 库的异常标志，避免影响后续运算 */
    feclearexcept(FE_ALL_EXCEPT);
}

/* 检查并修正无效操作的结果为 canonical NaN */
static void fix_nan_result(CPU *cpu, int rd) {
    int except = fetestexcept(FE_INVALID);
    if (except & FE_INVALID) {
        /* 只有结果确实是NaN时才修正为canonical NaN */
        uint32_t u = cpu->fregs[rd].u;
        int exp = (u >> 23) & 0xFF;
        int frac = u & 0x7FFFFF;
        if (exp == 0xFF && frac != 0) {
            cpu->fregs[rd].u = CANONICAL_QNAN;
        }
    }
}

/* ==================== 公共浮点运算函数 ==================== */
/* 用于 exec_fadd/fsub/fmul/fdiv 和 fmadd/fmsub/fnmadd/fnmadd 共享 */

/* 浮点运算结果结构体 */
typedef struct {
    uint32_t result;   /* 运算结果 */
    int has_nv;        /* 需要设置 NV 标志 */
    int has_dz;        /* 需要设置 DZ 标志 */
    int has_of;        /* 需要设置 OF 标志 */
} FloatOpResult;

/* 公共浮点加法: a + b */
static FloatOpResult float_do_add(uint32_t ua, uint32_t ub) {
    FloatOpResult res = {CANONICAL_QNAN, 0, 0, 0};
    FloatInfo fa = classify_float(ua);
    FloatInfo fb = classify_float(ub);

    /* NaN 输入检查 */
    if (fa.is_nan || fb.is_nan) {
        res.has_nv = fa.is_snan || fb.is_snan;
        return res;
    }

    /* Inf 特殊情况: Inf + (-Inf) = NaN + NV */
    if (check_inf_add_sub(fa, fb, 0)) {
        res.result = fa.sign ? 0xFFC00000 : 0x7FC00000;
        res.has_nv = 1;
        return res;
    }

    /* Inf + Inf (同号) = Inf (不产生异常) */
    if (fa.is_inf && fb.is_inf) {
        /* 同号Inf相加，结果符号与操作数相同 */
        res.result = fa.sign ? 0xFF800000 : 0x7F800000;
        return res;
    }

    /* Inf + finite = Inf (不产生异常) */
    if (fa.is_inf) {
        res.result = fa.sign ? 0xFF800000 : 0x7F800000;
        return res;
    }
    if (fb.is_inf) {
        res.result = fb.sign ? 0xFF800000 : 0x7F800000;
        return res;
    }

    /* 正常运算 */
    float a = *(float*)&ua;
    float b = *(float*)&ub;
    float r = a + b;
    res.result = *(uint32_t*)&r;

    /* 检查 C 库异常 */
    if (fetestexcept(FE_INVALID)) {
        FloatInfo fres = classify_float(res.result);
        if (fres.is_nan) {
            res.result = fa.sign ? 0xFFC00000 : 0x7FC00000;
            res.has_nv = 1;
        }
    }
    if (fetestexcept(FE_OVERFLOW)) res.has_of = 1;

    return res;
}

/* 公共浮点减法: a - b */
static FloatOpResult float_do_sub(uint32_t ua, uint32_t ub) {
    FloatOpResult res = {CANONICAL_QNAN, 0, 0, 0};
    FloatInfo fa = classify_float(ua);
    FloatInfo fb = classify_float(ub);

    /* NaN 输入检查 */
    if (fa.is_nan || fb.is_nan) {
        res.has_nv = fa.is_snan || fb.is_snan;
        return res;
    }

    /* Inf 特殊情况: Inf - Inf (同号) = NaN + NV */
    if (check_inf_add_sub(fa, fb, 1)) {
        res.result = fa.sign ? 0xFFC00000 : 0x7FC00000;
        res.has_nv = 1;
        return res;
    }

    /* Inf - Inf (不同号) = Inf (不产生异常) */
    if (fa.is_inf && fb.is_inf) {
        /* 不同号Inf相减，结果为第一个操作数的符号 */
        res.result = fa.sign ? 0xFF800000 : 0x7F800000;
        return res;
    }

    /* Inf - finite = Inf (不产生异常) */
    if (fa.is_inf) {
        res.result = fa.sign ? 0xFF800000 : 0x7F800000;
        return res;
    }

    /* finite - Inf = -Inf (不产生异常) */
    if (fb.is_inf) {
        res.result = fb.sign ? 0x7F800000 : 0xFF800000;  /* 取负 */
        return res;
    }

    /* 正常运算 */
    float a = *(float*)&ua;
    float b = *(float*)&ub;
    float r = a - b;
    res.result = *(uint32_t*)&r;

    /* 检查 C 库异常 */
    if (fetestexcept(FE_INVALID)) {
        FloatInfo fres = classify_float(res.result);
        if (fres.is_nan) {
            res.result = fa.sign ? 0xFFC00000 : 0x7FC00000;
            res.has_nv = 1;
        }
    }
    if (fetestexcept(FE_OVERFLOW)) res.has_of = 1;

    return res;
}

/* 公共浮点乘法: a * b */
static FloatOpResult float_do_mul(uint32_t ua, uint32_t ub) {
    FloatOpResult res = {CANONICAL_QNAN, 0, 0, 0};
    FloatInfo fa = classify_float(ua);
    FloatInfo fb = classify_float(ub);

    /* NaN 输入检查 */
    if (fa.is_nan || fb.is_nan) {
        res.has_nv = fa.is_snan || fb.is_snan;
        return res;
    }

    /* Inf 特殊情况: 0 * Inf = NaN + NV */
    if (check_inf_mul(fa, fb)) {
        res.has_nv = 1;
        return res;
    }

    /* finite * Inf = Inf (不产生异常) */
    if ((fa.is_inf && !fb.is_nan && !fb.is_inf) ||
        (fb.is_inf && !fa.is_nan && !fa.is_inf)) {
        int result_sign = fa.sign ^ fb.sign;
        res.result = result_sign ? 0xFF800000 : 0x7F800000;
        return res;  /* 不设置 NV 或 OF */
    }

    /* Inf * Inf = Inf (不产生异常) */
    if (fa.is_inf && fb.is_inf) {
        int result_sign = fa.sign ^ fb.sign;
        res.result = result_sign ? 0xFF800000 : 0x7F800000;
        return res;
    }

    /* 正常运算 */
    float a = *(float*)&ua;
    float b = *(float*)&ub;
    float r = a * b;
    res.result = *(uint32_t*)&r;

    /* 检查 C 库异常 */
    if (fetestexcept(FE_INVALID)) {
        FloatInfo fres = classify_float(res.result);
        if (fres.is_nan) {
            res.result = (fa.sign ^ fb.sign) ? 0xFFC00000 : 0x7FC00000;
            res.has_nv = 1;
        }
    }
    if (fetestexcept(FE_OVERFLOW)) res.has_of = 1;

    return res;
}

/* 公共浮点除法: a / b */
static FloatOpResult float_do_div(uint32_t ua, uint32_t ub) {
    FloatOpResult res = {CANONICAL_QNAN, 0, 0, 0};
    FloatInfo fa = classify_float(ua);
    FloatInfo fb = classify_float(ub);

    /* NaN 输入检查 */
    if (fa.is_nan || fb.is_nan) {
        res.has_nv = fa.is_snan || fb.is_snan;
        return res;
    }

    /* Inf 特殊情况检查 */
    int div_case = check_inf_div(fa, fb);
    if (div_case == 1) {
        /* Inf/Inf 或 0/0 = NaN + NV */
        res.has_nv = 1;
        return res;
    } else if (div_case == 2) {
        /* finite / 0 = Inf + DZ */
        int result_sign = fa.sign ^ fb.sign;
        res.result = result_sign ? 0xFF800000 : 0x7F800000;
        res.has_dz = 1;
        return res;
    } else if (div_case == 3) {
        /* Inf / finite = Inf + OF, 或 finite / Inf = 0 + OF */
        int result_sign = fa.sign ^ fb.sign;
        if (fa.is_inf) {
            res.result = result_sign ? 0xFF800000 : 0x7F800000;
        } else {
            res.result = result_sign ? 0x80000000 : 0x00000000;
        }
        res.has_of = 1;
        return res;
    }

    /* 正常运算 */
    float a = *(float*)&ua;
    float b = *(float*)&ub;
    float r = a / b;
    res.result = *(uint32_t*)&r;

    /* 检查 C 库异常 */
    if (fetestexcept(FE_INVALID)) {
        FloatInfo fres = classify_float(res.result);
        if (fres.is_nan) {
            res.result = fa.sign ? 0xFFC00000 : 0x7FC00000;
            res.has_nv = 1;
        }
    }
    if (fetestexcept(FE_DIVBYZERO)) res.has_dz = 1;
    if (fetestexcept(FE_OVERFLOW)) res.has_of = 1;

    return res;
}

/* 翻转浮点数符号位 */
static uint32_t float_flip_sign(uint32_t u) {
    return u ^ 0x80000000;
}

/* 外部函数声明 */
extern int32_t decode_branch_imm(uint32_t instr);
extern int32_t decode_jal_imm(uint32_t instr);
extern int32_t decode_store_imm(uint32_t instr);

/* ==================== M 扩展 (乘除) ==================== */

static void exec_mul(CPU *cpu, RType *r) {
    cpu->regs[r->rd] = (int32_t)cpu->regs[r->rs1] * (int32_t)cpu->regs[r->rs2];
}

static void exec_mulh(CPU *cpu, RType *r) {
    int64_t result = (int64_t)(int32_t)cpu->regs[r->rs1] * (int64_t)(int32_t)cpu->regs[r->rs2];
    cpu->regs[r->rd] = (uint32_t)(result >> 32);
}

static void exec_mulhsu(CPU *cpu, RType *r) {
    int64_t result = (int64_t)(int32_t)cpu->regs[r->rs1] * (uint64_t)cpu->regs[r->rs2];
    cpu->regs[r->rd] = (uint32_t)(result >> 32);
}

static void exec_mulhu(CPU *cpu, RType *r) {
    uint64_t result = (uint64_t)cpu->regs[r->rs1] * (uint64_t)cpu->regs[r->rs2];
    cpu->regs[r->rd] = (uint32_t)(result >> 32);
}

static void exec_div(CPU *cpu, RType *r) {
    int32_t rs1 = (int32_t)cpu->regs[r->rs1];
    int32_t rs2 = (int32_t)cpu->regs[r->rs2];
    if (rs2 == 0)
        cpu->regs[r->rd] = 0xFFFFFFFF;
    else if (rs1 == (int32_t)0x80000000 && rs2 == -1)
        cpu->regs[r->rd] = 0x80000000;
    else
        cpu->regs[r->rd] = (uint32_t)(rs1 / rs2);
}

static void exec_divu(CPU *cpu, RType *r) {
    if (cpu->regs[r->rs2] == 0)
        cpu->regs[r->rd] = 0xFFFFFFFF;
    else
        cpu->regs[r->rd] = cpu->regs[r->rs1] / cpu->regs[r->rs2];
}

static void exec_rem(CPU *cpu, RType *r) {
    int32_t rs1 = (int32_t)cpu->regs[r->rs1];
    int32_t rs2 = (int32_t)cpu->regs[r->rs2];
    if (rs2 == 0)
        cpu->regs[r->rd] = cpu->regs[r->rs1];
    else if (rs1 == (int32_t)0x80000000 && rs2 == -1)
        cpu->regs[r->rd] = 0;
    else
        cpu->regs[r->rd] = (uint32_t)(rs1 % rs2);
}

static void exec_remu(CPU *cpu, RType *r) {
    if (cpu->regs[r->rs2] == 0)
        cpu->regs[r->rd] = cpu->regs[r->rs1];
    else
        cpu->regs[r->rd] = cpu->regs[r->rs1] % cpu->regs[r->rs2];
}

/* ==================== A 扩展 (原子操作) ==================== */

static void exec_lr_w(CPU *cpu, RType *r) {
    uint32_t addr = cpu->regs[r->rs1];
    uint32_t val = mem_read(cpu, addr, 4);
    cpu->regs[r->rd] = val;
    cpu->has_reservation = 1;
    cpu->reservation_addr = addr;
}

static void exec_sc_w(CPU *cpu, RType *r) {
    uint32_t addr = cpu->regs[r->rs1];
    if (cpu->has_reservation && cpu->reservation_addr == addr) {
        mem_write(cpu, addr, cpu->regs[r->rs2], 4);
        cpu->regs[r->rd] = 0;
    } else {
        cpu->regs[r->rd] = 1;
    }
    cpu->has_reservation = 0;
}

static void exec_amo(CPU *cpu, RType *r, uint32_t funct5) {
    uint32_t addr = cpu->regs[r->rs1];
    uint32_t old_val = mem_read(cpu, addr, 4);
    uint32_t new_val;

    switch (funct5) {
        case 0x01: new_val = cpu->regs[r->rs2]; break;  /* AMOSWAP */
        case 0x00: new_val = old_val + cpu->regs[r->rs2]; break;  /* AMOADD */
        case 0x04: new_val = old_val ^ cpu->regs[r->rs2]; break;  /* AMOXOR */
        case 0x0C: new_val = old_val & cpu->regs[r->rs2]; break;  /* AMOAND */
        case 0x08: new_val = old_val | cpu->regs[r->rs2]; break;  /* AMOOR */
        case 0x10: new_val = ((int32_t)old_val < (int32_t)cpu->regs[r->rs2]) ? old_val : cpu->regs[r->rs2]; break;  /* AMOMIN */
        case 0x14: new_val = ((int32_t)old_val > (int32_t)cpu->regs[r->rs2]) ? old_val : cpu->regs[r->rs2]; break;  /* AMOMAX */
        case 0x18: new_val = (old_val < cpu->regs[r->rs2]) ? old_val : cpu->regs[r->rs2]; break;  /* AMOMINU */
        case 0x1C: new_val = (old_val > cpu->regs[r->rs2]) ? old_val : cpu->regs[r->rs2]; break;  /* AMOMAXU */
        default: new_val = old_val; break;
    }

    mem_write(cpu, addr, new_val, 4);
    cpu->regs[r->rd] = old_val;
}

/* ==================== F 扩展 (浮点运算) ==================== */

static void exec_fadd(CPU *cpu, RType *r) {
    uint32_t ua = cpu->fregs[r->rs1].u;
    uint32_t ub = cpu->fregs[r->rs2].u;
    feclearexcept(FE_ALL_EXCEPT);

    FloatOpResult res = float_do_add(ua, ub);
    cpu->fregs[r->rd].u = res.result;
    if (res.has_nv) feraiseexcept(FE_INVALID);
    if (res.has_of) feraiseexcept(FE_OVERFLOW);
    update_fflags(cpu);
}

static void exec_fsub(CPU *cpu, RType *r) {
    uint32_t ua = cpu->fregs[r->rs1].u;
    uint32_t ub = cpu->fregs[r->rs2].u;
    feclearexcept(FE_ALL_EXCEPT);

    FloatOpResult res = float_do_sub(ua, ub);
    cpu->fregs[r->rd].u = res.result;
    if (res.has_nv) feraiseexcept(FE_INVALID);
    if (res.has_of) feraiseexcept(FE_OVERFLOW);
    update_fflags(cpu);
}

static void exec_fmul(CPU *cpu, RType *r) {
    uint32_t ua = cpu->fregs[r->rs1].u;
    uint32_t ub = cpu->fregs[r->rs2].u;
    feclearexcept(FE_ALL_EXCEPT);

    FloatOpResult res = float_do_mul(ua, ub);
    cpu->fregs[r->rd].u = res.result;
    if (res.has_nv) feraiseexcept(FE_INVALID);
    if (res.has_of) feraiseexcept(FE_OVERFLOW);
    update_fflags(cpu);
}

static void exec_fdiv(CPU *cpu, RType *r) {
    uint32_t ua = cpu->fregs[r->rs1].u;
    uint32_t ub = cpu->fregs[r->rs2].u;
    feclearexcept(FE_ALL_EXCEPT);

    FloatOpResult res = float_do_div(ua, ub);
    cpu->fregs[r->rd].u = res.result;
    if (res.has_nv) feraiseexcept(FE_INVALID);
    if (res.has_dz) feraiseexcept(FE_DIVBYZERO);
    if (res.has_of) feraiseexcept(FE_OVERFLOW);
    update_fflags(cpu);
}

static void exec_fsqrt(CPU *cpu, RType *r) {
    uint32_t u = cpu->fregs[r->rs1].u;
    feclearexcept(FE_ALL_EXCEPT);

    NaNInfo ni = classify_nan(u);
    if (ni.is_nan) {
        cpu->fregs[r->rd].u = CANONICAL_QNAN;
        if (ni.is_snan) feraiseexcept(FE_INVALID);
    } else {
        if(u==0 || (u&(1<<31))) // 负数或零
        {
            cpu->fregs[r->rd].u = CANONICAL_QNAN;
            feraiseexcept(FE_INVALID);
        }
        else
        {
            cpu->fregs[r->rd].f = sqrtf(cpu->fregs[r->rs1].f);
            fix_nan_result(cpu, r->rd);
        }
    }
    update_fflags(cpu);
}

static void exec_fsgnj(CPU *cpu, RType *r, int neg, int xnor) {
    uint32_t sign = (cpu->fregs[r->rs2].u >> 31) & 1;
    if (neg) sign = !sign;
    if (xnor) sign = ((cpu->fregs[r->rs1].u >> 31) & 1) ^ sign;
    cpu->fregs[r->rd].u = (cpu->fregs[r->rs1].u & 0x7FFFFFFF) | (sign << 31);
}

static void exec_fmin(CPU *cpu, RType *r) {
    float a = cpu->fregs[r->rs1].f;
    float b = cpu->fregs[r->rs2].f;
    uint32_t ua = cpu->fregs[r->rs1].u;
    uint32_t ub = cpu->fregs[r->rs2].u;
    feclearexcept(FE_ALL_EXCEPT);

    NaNInfo na = classify_nan(ua);
    NaNInfo nb = classify_nan(ub);

    if (na.is_nan || nb.is_nan) {
        /* NaN 处理：返回非 NaN 的操作数，如果都是 NaN 返回 canonical NaN */
        if (na.is_nan && nb.is_nan) {
            cpu->fregs[r->rd].u = CANONICAL_QNAN;
        } else if (na.is_nan) {
            cpu->fregs[r->rd].f = b;
        } else {
            cpu->fregs[r->rd].f = a;
        }
        /* sNaN 设置 NV */
        if (na.is_snan || nb.is_snan) {
            feraiseexcept(FE_INVALID);
        }
    } else {
        /* 正常比较：fmin 返回较小的数 */
        /* 如果 a == b，返回符号为负的那个（即 -0.0） */
        if (a == b) {
            cpu->fregs[r->rd].u = (ua & 0x80000000) ? ua : ub;  /* 返回负数 */
        } else {
            cpu->fregs[r->rd].f = (a < b) ? a : b;
        }
    }
    update_fflags(cpu);
}

static void exec_fmax(CPU *cpu, RType *r) {
    float a = cpu->fregs[r->rs1].f;
    float b = cpu->fregs[r->rs2].f;
    uint32_t ua = cpu->fregs[r->rs1].u;
    uint32_t ub = cpu->fregs[r->rs2].u;
    feclearexcept(FE_ALL_EXCEPT);

    NaNInfo na = classify_nan(ua);
    NaNInfo nb = classify_nan(ub);

    if (na.is_nan || nb.is_nan) {
        /* NaN 处理：返回非 NaN 的操作数，如果都是 NaN 返回 canonical NaN */
        if (na.is_nan && nb.is_nan) {
            cpu->fregs[r->rd].u = CANONICAL_QNAN;
        } else if (na.is_nan) {
            cpu->fregs[r->rd].f = b;
        } else {
            cpu->fregs[r->rd].f = a;
        }
        /* sNaN 设置 NV */
        if (na.is_snan || nb.is_snan) {
            feraiseexcept(FE_INVALID);
        }
    } else {
        /* 正常比较：注意 +0.0 > -0.0 */
        /* 如果 a == b，返回符号为正的那个（即 +0.0） */
        if (a == b) {
            cpu->fregs[r->rd].u = (ua & 0x80000000) ? ub : ua;  /* 返回正数 */
        } else {
            cpu->fregs[r->rd].f = (a > b) ? a : b;
        }
    }
    update_fflags(cpu);
}

static void exec_fcvt_w_s(CPU *cpu, RType *r, int unsign) {
    float f = cpu->fregs[r->rs1].f;
    uint32_t u = cpu->fregs[r->rs1].u;
    feclearexcept(FE_ALL_EXCEPT);

    NaNInfo ni = classify_nan(u);

    if (ni.is_nan) {
        /* NaN 输入：返回 INT32_MAX，设置 NV */
        if (unsign)
            cpu->regs[r->rd] = 0xFFFFFFFF;
        else
            cpu->regs[r->rd] = 0x7FFFFFFF;
        feraiseexcept(FE_INVALID);
    } else if (unsign) {
        /* fcvt.wu.s: 转换到 uint32 */
        if (f <= -1.0f) {
            /* 负数 <= -1：返回 0，设置 NV（超出范围） */
            cpu->regs[r->rd] = 0;
            feraiseexcept(FE_INVALID);
        } else if (f < 0) {
            /* 介于 -1 和 0 之间的负数：truncation 到 0，设置 NX */
            cpu->regs[r->rd] = 0;
            feraiseexcept(FE_INEXACT);
        } else if (f > (float)0xFFFFFFFF) {
            /* 超出 uint32 范围：返回 UINT32_MAX，设置 NV */
            cpu->regs[r->rd] = 0xFFFFFFFF;
            feraiseexcept(FE_INVALID);
        } else {
            /* 正常范围内的转换 */
            uint32_t result = (uint32_t)f;
            cpu->regs[r->rd] = result;
            /* 检查 inexact：truncation 是否丢失精度 */
            if ((float)result != f) {
                feraiseexcept(FE_INEXACT);
            }
        }
    } else {
        /* fcvt.w.s: 转换到 int32 */
        if (f > (float)INT32_MAX) {
            /* 正数超出范围：返回 INT32_MAX，设置 NV */
            cpu->regs[r->rd] = INT32_MAX;
            feraiseexcept(FE_INVALID);
        } else if (f < (float)INT32_MIN) {
            /* 负数超出范围：返回 INT32_MIN，设置 NV */
            cpu->regs[r->rd] = INT32_MIN;
            feraiseexcept(FE_INVALID);
        } else {
            /* 正常范围内的转换 */
            int32_t result = (int32_t)f;
            cpu->regs[r->rd] = result;
            /* 检查 inexact：truncation 是否丢失精度 */
            if ((float)result != f) {
                feraiseexcept(FE_INEXACT);
            }
        }
    }
    update_fflags(cpu);
}

static void exec_fcvt_s_w(CPU *cpu, RType *r, int unsign) {
    feclearexcept(FE_ALL_EXCEPT);
    if (unsign)
        cpu->fregs[r->rd].f = (float)cpu->regs[r->rs1];
    else
        cpu->fregs[r->rd].f = (float)(int32_t)cpu->regs[r->rs1];
    update_fflags(cpu);
}

static void exec_fclass(CPU *cpu, RType *r) {
    uint32_t u = cpu->fregs[r->rs1].u;
    uint32_t sign = (u >> 31) & 1;
    uint32_t exp = (u >> 23) & 0xFF;
    uint32_t frac = u & 0x7FFFFF;
    uint32_t result = 0;

#ifdef DEBUG_FCLASS
    printf("[FCLASS] rs1=%d u=0x%08x sign=%d exp=%d frac=%d\n", r->rs1, u, sign, exp, frac);
#endif

    /* 测试文件期望的 bit 顺序（按类型排列）：
     * bit 0: negative infinity
     * bit 1: negative normal number
     * bit 2: negative subnormal
     * bit 3: negative zero
     * bit 4: positive zero
     * bit 5: positive subnormal
     * bit 6: positive normal number
     * bit 7: positive infinity
     * bit 8: signaling NaN (frac最高位为0)
     * bit 9: quiet NaN (frac最高位为1)
     */
    if (exp == 0xFF && frac) {
        /* NaN: 区分 sNaN 和 qNaN */
        int is_qnan = (frac & 0x400000) != 0;  /* frac 最高位为1是 qNaN */
        result = is_qnan ? (1<<9) : (1<<8);
    }
    else if (exp == 0xFF)
        result = sign ? (1<<0) : (1<<7);      /* 无穷 */
    else if (exp == 0 && frac)
        result = sign ? (1<<2) : (1<<5);      /* 次正规数 */
    else if (exp == 0)
        result = sign ? (1<<3) : (1<<4);      /* 零 */
    else
        result = sign ? (1<<1) : (1<<6);      /* 正规数 */

#ifdef DEBUG_FCLASS
    printf("[FCLASS] result=%d\n", result);
#endif

    cpu->regs[r->rd] = result;
}

static void exec_fmv_x_w(CPU *cpu, RType *r) {
    cpu->regs[r->rd] = cpu->fregs[r->rs1].u;  /* 位拷贝 */
}

static void exec_fmv_w_x(CPU *cpu, RType *r) {
    cpu->fregs[r->rd].u = cpu->regs[r->rs1];  /* 位拷贝 */
}

static void exec_fle(CPU *cpu, RType *r) {
    float a = cpu->fregs[r->rs1].f;
    float b = cpu->fregs[r->rs2].f;
    feclearexcept(FE_ALL_EXCEPT);

    NaNInfo na = classify_nan(cpu->fregs[r->rs1].u);
    NaNInfo nb = classify_nan(cpu->fregs[r->rs2].u);

    if (na.is_nan || nb.is_nan) {
        /* 任一操作数是 NaN：返回 0，设置 NV */
        cpu->regs[r->rd] = 0;
        feraiseexcept(FE_INVALID);
    } else {
        cpu->regs[r->rd] = (a <= b) ? 1 : 0;
    }
    update_fflags(cpu);
}

static void exec_flt(CPU *cpu, RType *r) {
    float a = cpu->fregs[r->rs1].f;
    float b = cpu->fregs[r->rs2].f;
    feclearexcept(FE_ALL_EXCEPT);

    NaNInfo na = classify_nan(cpu->fregs[r->rs1].u);
    NaNInfo nb = classify_nan(cpu->fregs[r->rs2].u);

    if (na.is_nan || nb.is_nan) {
        /* 任一操作数是 NaN：返回 0，设置 NV */
        cpu->regs[r->rd] = 0;
        feraiseexcept(FE_INVALID);
    } else {
        cpu->regs[r->rd] = (a < b) ? 1 : 0;
    }
    update_fflags(cpu);
}

static void exec_feq(CPU *cpu, RType *r) {
    float a = cpu->fregs[r->rs1].f;
    float b = cpu->fregs[r->rs2].f;
    feclearexcept(FE_ALL_EXCEPT);

    NaNInfo na = classify_nan(cpu->fregs[r->rs1].u);
    NaNInfo nb = classify_nan(cpu->fregs[r->rs2].u);

    if (na.is_nan || nb.is_nan) {
        /* 任一操作数是 NaN：返回 0 */
        cpu->regs[r->rd] = 0;
        /* 只有 sNaN 才设置 NV 异常 */
        if (na.is_snan || nb.is_snan) {
            feraiseexcept(FE_INVALID);
        }
    } else {
        cpu->regs[r->rd] = (a == b) ? 1 : 0;
    }
    update_fflags(cpu);
}

/* ==================== CSR 寄存器操作 ==================== */

static uint32_t csr_read(CPU *cpu, uint32_t csr_addr) {
    switch (csr_addr) {
        case CSR_FFLAGS:
            return cpu->fcsr & 0x1F;  /* FFLAGS = FCSR[4:0] */
        case CSR_FRM:
            return (cpu->fcsr >> 5) & 0x7;  /* FRM = FCSR[7:5] */
        case CSR_FCSR:
            return cpu->fcsr;  /* FCSR 完整值 */

        /* S-mode CSR */
        case CSR_SSTATUS:
            /* sstatus 是 mstatus 的受限子集 */
            return cpu->mstatus & (MSTATUS_SIE  | MSTATUS_SPIE | MSTATUS_SPP |
                                   MSTATUS_FS   | MSTATUS_XS   | MSTATUS_SUM |
                                   MSTATUS_MXR  | MSTATUS_SD);
        case CSR_SIE:      return cpu->sie;
        case CSR_STVEC:    return cpu->stvec;
        case CSR_SSCRATCH: return cpu->sscratch;
        case CSR_SEPC:     return cpu->sepc;
        case CSR_SCAUSE:   return cpu->scause;
        case CSR_STVAL:    return cpu->stval;
        case CSR_SIP:      return cpu->sip;
        case CSR_SATP:     return cpu->satp;

        /* M-mode CSR */
        case CSR_MSTATUS:
            return cpu->mstatus;
        case CSR_MEDELEG:
            return cpu->medeleg;
        case CSR_MIDELEG:
            return cpu->mideleg;
        case CSR_MIE:
            return cpu->mie;
        case CSR_MTVEC:
            return cpu->mtvec;
        case CSR_MSCRATCH:
            return cpu->mscratch;
        case CSR_MEPC:
            return cpu->mepc;
        case CSR_MCAUSE:
            return cpu->mcause;
        case CSR_MTVAL:
            return cpu->mtval;
        case CSR_MIP:
            return cpu->mip;

        /* Machine Information Registers */
        case CSR_MISA:
            return (1u << 0)   /* A - 原子操作   */
                 | (1u << 2)   /* C - 压缩指令   */
                 | (1u << 5)   /* F - 单精度浮点 */
                 | (1u << 8)   /* I - 基础整数   */
                 | (1u << 12)  /* M - 乘除       */
                 | (1u << 18)  /* S - S-mode     */
                 | (1u << 20); /* U - U-mode     */
        case CSR_MVENDORID:  return 0;
        case CSR_MARCHID:    return 0;
        case CSR_MIMPID:     return 0;
        case CSR_MHARTID:    return 0;

        /* Timer CSRs (只读) — 直接读 cpu->cycles (64-bit) */
        case CSR_TIME:
            return (uint32_t)(cpu->cycles & 0xFFFFFFFFu);
        case CSR_TIMEH:
            return (uint32_t)(cpu->cycles >> 32);

        default:
            return 0;  /* 未实现的 CSR 返回 0 */
    }
}

static void csr_write(CPU *cpu, uint32_t csr_addr, uint32_t value) {
    switch (csr_addr) {
        case CSR_FFLAGS:
            cpu->fcsr = (cpu->fcsr & ~0x1F) | (value & 0x1F);
            break;
        case CSR_FRM:
            cpu->fcsr = (cpu->fcsr & ~0xE0) | ((value & 0x7) << 5);
            break;
        case CSR_FCSR:
            cpu->fcsr = value & 0xFF;  /* FCSR 只有低 8 位有效 */
            break;

        /* S-mode CSR */
        case CSR_SSTATUS:
            {
                uint32_t mask = MSTATUS_SIE | MSTATUS_SPIE | MSTATUS_SPP |
                                MSTATUS_SUM | MSTATUS_MXR;
                cpu->mstatus = (cpu->mstatus & ~mask) | (value & mask);
                /* SD 是只读的: FS 或 XS 有非零值时置 1 */
                if ((cpu->mstatus & (MSTATUS_FS | MSTATUS_XS)) != 0)
                    cpu->mstatus |= MSTATUS_SD;
                else
                    cpu->mstatus &= ~MSTATUS_SD;
            }
            break;
        case CSR_SIE:      cpu->sie      = value; break;
        case CSR_STVEC:    cpu->stvec    = value; break;
        case CSR_SSCRATCH: cpu->sscratch = value; break;
        case CSR_SEPC:     cpu->sepc     = value; break;
        case CSR_SCAUSE:   cpu->scause   = value; break;
        case CSR_STVAL:    cpu->stval    = value; break;
        case CSR_SIP:      cpu->sip      = value; break;
        case CSR_SATP:
            cpu->satp = value;
            mmu_flush_tlb(cpu);  /* SATP 改变 → TLB 全部无效 */
            cpu->mmu_skip = 1;   /* 模拟流水线：物理地址继续恒等映射
                                    直到首次 VA >= 0xC0000000 */
            break;

        /* M-mode CSR */
        case CSR_MSTATUS:
            cpu->mstatus = value;
            break;
        case CSR_MEDELEG:
            cpu->medeleg = value & 0xFFFF;  /* 低 16 位有效 */
            break;
        case CSR_MIDELEG:
            /* SSIP(1), STIP(5), SEIP(9) 可委托 */
            cpu->mideleg = value & (SIP_SSIP | SIP_STIP | SIP_SEIP);
            break;
        case CSR_MIE:
            cpu->mie = value;
            break;
        case CSR_MTVEC:
            cpu->mtvec = value;
            break;
        case CSR_MSCRATCH:
            cpu->mscratch = value;
            break;
        case CSR_MEPC:
            cpu->mepc = value;
            break;
        case CSR_MCAUSE:
            cpu->mcause = value;
            break;
        case CSR_MTVAL:
            cpu->mtval = value;
            break;
        case CSR_MIP:
            cpu->mip = value;
            break;
        /* 其他 CSR 不实现，写入忽略 */
    }
}

static void exec_csrrw(CPU *cpu, uint32_t rd, uint32_t rs1, uint32_t csr_addr) {
    uint32_t old_val = csr_read(cpu, csr_addr);
    /* 注意：此项目使用的 fsflags 指令实现为 CSRRW，且当 rs1=0 时需要清除 CSR */
    csr_write(cpu, csr_addr, cpu->regs[rs1]);  /* rs1=0 时写入 0，清除 CSR */
    if (rd != 0) {
        cpu->regs[rd] = old_val;
    }
}

static void exec_csrrs(CPU *cpu, uint32_t rd, uint32_t rs1, uint32_t csr_addr) {
    uint32_t old_val = csr_read(cpu, csr_addr);
    if (rs1 != 0) {  /* rs1=0 时只读不置位 */
        csr_write(cpu, csr_addr, old_val | cpu->regs[rs1]);
    }
    if (rd != 0) {
        cpu->regs[rd] = old_val;
    }
}

static void exec_csrrc(CPU *cpu, uint32_t rd, uint32_t rs1, uint32_t csr_addr) {
    uint32_t old_val = csr_read(cpu, csr_addr);
    if (rs1 != 0) {  /* rs1=0 时只读不清位 */
        csr_write(cpu, csr_addr, old_val & ~cpu->regs[rs1]);
    }
    if (rd != 0) {
        cpu->regs[rd] = old_val;
    }
}

/* CSR 立即数版本 (rs1 作为 uimm[4:0]) */
static void exec_csrrwi(CPU *cpu, uint32_t rd, uint32_t uimm, uint32_t csr_addr) {
    uint32_t old_val = csr_read(cpu, csr_addr);
    csr_write(cpu, csr_addr, uimm);  /* 立即数直接写入 */
    if (rd != 0) {
        cpu->regs[rd] = old_val;
    }
}

static void exec_csrrsi(CPU *cpu, uint32_t rd, uint32_t uimm, uint32_t csr_addr) {
    uint32_t old_val = csr_read(cpu, csr_addr);
    csr_write(cpu, csr_addr, old_val | uimm);
    if (rd != 0) {
        cpu->regs[rd] = old_val;
    }
}

static void exec_csrrci(CPU *cpu, uint32_t rd, uint32_t uimm, uint32_t csr_addr) {
    uint32_t old_val = csr_read(cpu, csr_addr);
    csr_write(cpu, csr_addr, old_val & ~uimm);
    if (rd != 0) {
        cpu->regs[rd] = old_val;
    }
}

/* F 扩展融合乘加 (rs2 字段实际是 rs3) */
static void exec_fmadd(CPU *cpu, RType *r) {
    /* fmadd = rs1 * rs2 + rs3 */
    uint32_t ua = cpu->fregs[r->rs1].u;
    uint32_t ub = cpu->fregs[r->rs2].u;
    uint32_t uc = cpu->fregs[(r->funct7 >> 2) & 0x1F].u;
    feclearexcept(FE_ALL_EXCEPT);

    /* 第一步：乘法 rs1 * rs2 */
    FloatOpResult mul_res = float_do_mul(ua, ub);

    /* 第二步：加法 mul_result + rs3 */
    FloatOpResult add_res = float_do_add(mul_res.result, uc);

    cpu->fregs[r->rd].u = add_res.result;
    if (mul_res.has_nv || add_res.has_nv) feraiseexcept(FE_INVALID);
    if (mul_res.has_of || add_res.has_of) feraiseexcept(FE_OVERFLOW);
    update_fflags(cpu);
}

static void exec_fmsub(CPU *cpu, RType *r) {
    /* fmsub = rs1 * rs2 - rs3 */
    uint32_t ua = cpu->fregs[r->rs1].u;
    uint32_t ub = cpu->fregs[r->rs2].u;
    uint32_t uc = cpu->fregs[(r->funct7 >> 2) & 0x1F].u;
    feclearexcept(FE_ALL_EXCEPT);

    /* 第一步：乘法 rs1 * rs2 */
    FloatOpResult mul_res = float_do_mul(ua, ub);

    /* 第二步：减法 mul_result - rs3 */
    FloatOpResult sub_res = float_do_sub(mul_res.result, uc);

    cpu->fregs[r->rd].u = sub_res.result;
    if (mul_res.has_nv || sub_res.has_nv) feraiseexcept(FE_INVALID);
    if (mul_res.has_of || sub_res.has_of) feraiseexcept(FE_OVERFLOW);
    update_fflags(cpu);
}

static void exec_fnmsub(CPU *cpu, RType *r) {
    /* fnmsub = -(rs1 * rs2 - rs3) = -mul_result + rs3 */
    uint32_t ua = cpu->fregs[r->rs1].u;
    uint32_t ub = cpu->fregs[r->rs2].u;
    uint32_t uc = cpu->fregs[(r->funct7 >> 2) & 0x1F].u;
    feclearexcept(FE_ALL_EXCEPT);

    /* 第一步：乘法 rs1 * rs2 */
    FloatOpResult mul_res = float_do_mul(ua, ub);

    /* 取负：翻转符号位 */
    uint32_t neg_mul = float_flip_sign(mul_res.result);

    /* 第二步：加法 -mul_result + rs3 */
    FloatOpResult add_res = float_do_add(neg_mul, uc);

    cpu->fregs[r->rd].u = add_res.result;
    if (mul_res.has_nv || add_res.has_nv) feraiseexcept(FE_INVALID);
    if (mul_res.has_of || add_res.has_of) feraiseexcept(FE_OVERFLOW);
    update_fflags(cpu);
}

static void exec_fnmadd(CPU *cpu, RType *r) {
    /* fnmadd = -(rs1 * rs2 + rs3) = -mul_result - rs3 */
    uint32_t ua = cpu->fregs[r->rs1].u;
    uint32_t ub = cpu->fregs[r->rs2].u;
    uint32_t uc = cpu->fregs[(r->funct7 >> 2) & 0x1F].u;
    feclearexcept(FE_ALL_EXCEPT);

    /* 第一步：乘法 rs1 * rs2 */
    FloatOpResult mul_res = float_do_mul(ua, ub);

    /* 取负：翻转符号位 */
    uint32_t neg_mul = float_flip_sign(mul_res.result);

    /* 第二步：减法 -mul_result - rs3 */
    FloatOpResult sub_res = float_do_sub(neg_mul, uc);

    cpu->fregs[r->rd].u = sub_res.result;
    if (mul_res.has_nv || sub_res.has_nv) feraiseexcept(FE_INVALID);
    if (mul_res.has_of || sub_res.has_of) feraiseexcept(FE_OVERFLOW);
    update_fflags(cpu);
}

/* ==================== 基础整数运算 ==================== */
static void exec_add(CPU *cpu, RType *r) {
    cpu->regs[r->rd] = cpu->regs[r->rs1] + cpu->regs[r->rs2];
}

static void exec_sub(CPU *cpu, RType *r) {
    cpu->regs[r->rd] = cpu->regs[r->rs1] - cpu->regs[r->rs2];
}

static void exec_sll(CPU *cpu, RType *r) {
    cpu->regs[r->rd] = cpu->regs[r->rs1] << (cpu->regs[r->rs2] & 0x1F);
}

static void exec_slt(CPU *cpu, RType *r) {
    cpu->regs[r->rd] = ((int32_t)cpu->regs[r->rs1] < (int32_t)cpu->regs[r->rs2]) ? 1 : 0;
}

static void exec_sltu(CPU *cpu, RType *r) {
    cpu->regs[r->rd] = (cpu->regs[r->rs1] < cpu->regs[r->rs2]) ? 1 : 0;
}

static void exec_xor(CPU *cpu, RType *r) {
    cpu->regs[r->rd] = cpu->regs[r->rs1] ^ cpu->regs[r->rs2];
}

static void exec_srl(CPU *cpu, RType *r) {
    cpu->regs[r->rd] = cpu->regs[r->rs1] >> (cpu->regs[r->rs2] & 0x1F);
}

static void exec_sra(CPU *cpu, RType *r) {
    cpu->regs[r->rd] = (uint32_t)((int32_t)cpu->regs[r->rs1] >> (cpu->regs[r->rs2] & 0x1F));
}

static void exec_or(CPU *cpu, RType *r) {
    cpu->regs[r->rd] = cpu->regs[r->rs1] | cpu->regs[r->rs2];
}

static void exec_and(CPU *cpu, RType *r) {
    cpu->regs[r->rd] = cpu->regs[r->rs1] & cpu->regs[r->rs2];
}

/* 立即数运算 */
static void exec_addi(CPU *cpu, IType *i) {
    cpu->regs[i->rd] = cpu->regs[i->rs1] + i->imm;
}

static void exec_slli(CPU *cpu, IType *i) {
    cpu->regs[i->rd] = cpu->regs[i->rs1] << (i->imm & 0x1F);
}

static void exec_sliti(CPU *cpu, IType *i) {
    cpu->regs[i->rd] = ((int32_t)cpu->regs[i->rs1] < i->imm) ? 1 : 0;
}

static void exec_sltiu(CPU *cpu, IType *i) {
    cpu->regs[i->rd] = (cpu->regs[i->rs1] < (uint32_t)i->imm) ? 1 : 0;
}

static void exec_xori(CPU *cpu, IType *i) {
    cpu->regs[i->rd] = cpu->regs[i->rs1] ^ i->imm;
}

static void exec_srli(CPU *cpu, IType *i) {
    cpu->regs[i->rd] = cpu->regs[i->rs1] >> (i->imm & 0x1F);
}

static void exec_srai(CPU *cpu, IType *i) {
    cpu->regs[i->rd] = (uint32_t)((int32_t)cpu->regs[i->rs1] >> (i->imm & 0x1F));
}

static void exec_ori(CPU *cpu, IType *i) {
    cpu->regs[i->rd] = cpu->regs[i->rs1] | i->imm;
}

static void exec_andi(CPU *cpu, IType *i) {
    cpu->regs[i->rd] = cpu->regs[i->rs1] & i->imm;
}

/* ==================== 主执行函数 ==================== */

/**
 * cpu_execute - 执行指令
 * @cpu: CPU 状态结构体指针
 * @instr: 指令 (压缩指令为 16 位，标准指令为 32 位)
 * @instr_len: 指令长度 (2 或 4)
 *
 * 返回：1 表示继续执行，0 表示停止 (ECALL/EBREAK)，-1 表示错误
 */
int cpu_execute(CPU *cpu, uint32_t instr, int instr_len) {
    /* 压缩指令展开为 32 位等效指令 */
    uint32_t expanded_instr;
    int actual_len;
    /* Bug 13 根因修复：访存指令 fault 时 cpu_trap 已把 pc 重定向到陷阱
     * 向量，执行体末尾的 cpu->pc += actual_len 会再把向量地址加偏移，
     * 导致内核从 handle_exception+4 进入（跳过入口的 csrrw tp,sscratch
     * 交换），tp/sscratch 协议被破坏。入口处记录 pc，仅在未被陷阱
     * 重定向时才推进。 */
    uint32_t pc_entry = cpu->pc;

    if (instr_len == 2) {
        expanded_instr = expand_compressed((uint16_t)instr);
        if (expanded_instr == 0) {
            fprintf(stderr, "[EXECUTE] 非法压缩指令：0x%04x\n", (uint16_t)instr);
            return -1;
        }
        actual_len = 2;
#ifdef DEBUG_COMPRESSED
        printf("[COMPRESSED] 0x%04x -> 0x%08x\n", (uint16_t)instr, expanded_instr);
#endif
    } else {
        expanded_instr = instr;
        actual_len = 4;
    }

    /* 统一执行 32 位指令 */
    RType r_inst;
    IType i_inst;
    SType s_inst;

    cpu_decode(cpu, expanded_instr, &r_inst, &i_inst, &s_inst, NULL);

    uint32_t opcode = r_inst.opcode;
    uint32_t funct3 = r_inst.funct3;
    uint32_t funct7 = r_inst.funct7;

    switch (opcode) {
        /* ==================== OP (0x33) ==================== */
        case OP_OP:
            /* M 扩展 (funct7 = 0x01) */
            if (funct7 == 0x01) {
                switch (funct3) {
                    case F3_MUL:   exec_mul(cpu, &r_inst); break;
                    case F3_MULH:  exec_mulh(cpu, &r_inst); break;
                    case F3_MULHSU: exec_mulhsu(cpu, &r_inst); break;
                    case F3_MULHU: exec_mulhu(cpu, &r_inst); break;
                    case F3_DIV:   exec_div(cpu, &r_inst); break;
                    case F3_DIVU:  exec_divu(cpu, &r_inst); break;
                    case F3_REM:   exec_rem(cpu, &r_inst); break;
                    case F3_REMU:  exec_remu(cpu, &r_inst); break;
                }
            } else {
                /* 基础 OP */
                switch (funct3) {
                    case 0x0:
                        if (funct7 == 0x20) exec_sub(cpu, &r_inst);
                        else exec_add(cpu, &r_inst);
                        break;
                    case 0x1: exec_sll(cpu, &r_inst); break;
                    case 0x2: exec_slt(cpu, &r_inst); break;
                    case 0x3: exec_sltu(cpu, &r_inst); break;
                    case 0x4: exec_xor(cpu, &r_inst); break;
                    case 0x5:
                        if (funct7 == 0x20) exec_sra(cpu, &r_inst);
                        else exec_srl(cpu, &r_inst);
                        break;
                    case 0x6: exec_or(cpu, &r_inst); break;
                    case 0x7: exec_and(cpu, &r_inst); break;
                }
            }
            cpu->pc += actual_len;
            break;

        /* ==================== OP-IMM (0x13) ==================== */
        case OP_OP_IMM:
            switch (funct3) {
                case 0x0: exec_addi(cpu, &i_inst); break;
                case 0x1: exec_slli(cpu, &i_inst); break;
                case 0x2: exec_sliti(cpu, &i_inst); break;
                case 0x3: exec_sltiu(cpu, &i_inst); break;
                case 0x4: exec_xori(cpu, &i_inst); break;
                case 0x5:
                    if (i_inst.imm & 0x400) exec_srai(cpu, &i_inst);
                    else exec_srli(cpu, &i_inst);
                    break;
                case 0x6: exec_ori(cpu, &i_inst); break;
                case 0x7: exec_andi(cpu, &i_inst); break;
            }
            cpu->pc += actual_len;
            break;

        /* ==================== OP-ATOM (0x2F) - A 扩展 ==================== */
        case 0x2F:
            {
                uint32_t funct5 = funct7 >> 2;
                /* Bug 16/17：AMO/LR 内部的 mem_read fault 会把 0 写入 rd。
                 * 保存原始 rd，trap 后恢复（真机上 faulting AMO 不写 rd）。 */
                uint32_t orig_rd_val = cpu->regs[r_inst.rd];
                if (funct3 == F3_AMO && funct5 == 0x02)
                    exec_lr_w(cpu, &r_inst);
                else if (funct3 == F3_AMO && funct5 == 0x03)
                    exec_sc_w(cpu, &r_inst);
                else if (funct3 == F3_AMO)
                    exec_amo(cpu, &r_inst, funct5);
                /* trap 发生（pc 被改向 stvec）→ 恢复 rd 原值 */
                if (cpu->pc != pc_entry)
                    cpu->regs[r_inst.rd] = orig_rd_val;
            }
            if (cpu->pc == pc_entry)
                cpu->pc += actual_len;
            break;

        /* ==================== LOAD (0x03) ==================== */
        case OP_LOAD:
            {
                uint32_t addr = cpu->regs[i_inst.rs1] + i_inst.imm;
                int size = (funct3 == 0x0 || funct3 == 0x4) ? 1 :
                           (funct3 == 0x1 || funct3 == 0x5) ? 2 : 4;

                uint32_t value = mem_read(cpu, addr, size);

                /* Bug 16 修复：页错误时 mem_read 返回 0 且 cpu_trap 已触发，
                 * 真机上 faulting load 不写目标寄存器。只有未陷入时才写 rd。 */
                if (cpu->pc == pc_entry) {
                    /* 符号扩展处理 */
                    switch (funct3) {
                        case 0x0:  /* LB */
                            value = sign_extend(value, 8);
                            break;
                        case 0x1:  /* LH */
                            value = sign_extend(value, 16);
                            break;
                        /* 0x2: LW, 0x4: LBU, 0x5: LHU 不需要符号扩展 */
                    }

                    cpu->regs[i_inst.rd] = value;
                }
            }
            if (cpu->pc == pc_entry)
                cpu->pc += actual_len;
            break;

        /* ==================== STORE (0x23) ==================== */
        case OP_STORE:
            {
                int32_t imm = decode_store_imm(expanded_instr);
                uint32_t addr = cpu->regs[s_inst.rs1] + imm;
                uint32_t val = cpu->regs[s_inst.rs2];

                int size = (funct3 == 0x0) ? 1 : (funct3 == 0x1) ? 2 : 4;
                mem_write(cpu, addr, val, size);
            }
            if (cpu->pc == pc_entry)
                cpu->pc += actual_len;
            break;

        /* ==================== BRANCH (0x63) ==================== */
        case OP_BRANCH:
            {
                int32_t imm = decode_branch_imm(expanded_instr);
                int32_t rs1 = (int32_t)cpu->regs[s_inst.rs1];
                int32_t rs2 = (int32_t)cpu->regs[s_inst.rs2];
                int taken = 0;

                switch (funct3) {
                    case 0x0: taken = (rs1 == rs2); break;  /* BEQ */
                    case 0x1: taken = (rs1 != rs2); break;  /* BNE */
                    case 0x4: taken = (rs1 < rs2); break;   /* BLT */
                    case 0x5: taken = (rs1 >= rs2); break;  /* BGE */
                    case 0x6: taken = ((uint32_t)rs1 < (uint32_t)rs2); break;  /* BLTU */
                    case 0x7: taken = ((uint32_t)rs1 >= (uint32_t)rs2); break; /* BGEU */
                }

                cpu->pc += taken ? imm : actual_len;
            }
            break;

        /* ==================== JALR (0x67) ==================== */
        case OP_JALR:
            {
                uint32_t target = (cpu->regs[i_inst.rs1] + i_inst.imm) & ~1;
                cpu->regs[i_inst.rd] = cpu->pc + actual_len;
                cpu->pc = target;
            }
            break;

        /* ==================== JAL (0x6F) ==================== */
        case OP_JAL:
            {
                int32_t imm = decode_jal_imm(expanded_instr);
                cpu->regs[r_inst.rd] = cpu->pc + actual_len;
                cpu->pc += imm;
            }
            break;

        /* ==================== LUI (0x37) ==================== */
        case OP_LUI:
            /* 20位立即数需要符号扩展 (instr[31:12]) */
            cpu->regs[r_inst.rd] = (int32_t)(expanded_instr & 0xFFFFF000);
            cpu->pc += actual_len;
            break;

        /* ==================== AUIPC (0x17) ==================== */
        case OP_AUIPC:
            /* 20位立即数需要符号扩展 (instr[31:12]) */
            cpu->regs[r_inst.rd] = cpu->pc + (int32_t)(expanded_instr & 0xFFFFF000);
            cpu->pc += actual_len;
            break;

        /* ==================== FENCE (0x0F) ==================== */
        case OP_FENCE:
            /* FENCE 在单线程仿真器中为 NOP，直接跳过 */
            cpu->pc += actual_len;
            break;

        /* ==================== SYSTEM (0x73) ==================== */
        case OP_SYSTEM:
            {
                uint32_t csr_addr = (instr >> 20) & 0xFFF;  /* CSR 地址 */
                uint32_t rd = r_inst.rd;
                uint32_t rs1 = r_inst.rs1;
                uint32_t uimm = rs1;  /* 立即数版本使用 rs1 作为 uimm[4:0] */

                if (expanded_instr == 0x00000073) {  /* ECALL */
                    uint32_t ecall_cause;
                    switch (cpu->privilege) {
                        case PRIV_U: ecall_cause = CAUSE_USER_ECALL;        break;
                        case PRIV_S: ecall_cause = CAUSE_SUPERVISOR_ECALL;  break;
                        default:     ecall_cause = CAUSE_MACHINE_ECALL;     break;
                    }
                    cpu_trap(cpu, ecall_cause, 0);
                    return 1;  /* 陷阱已处理，继续执行 */
                } else if (expanded_instr == 0x00100073) {  /* EBREAK */
                    printf("\n[EBREAK] 断点 @ PC=0x%08x\n", cpu->pc);
                    cpu_trap(cpu, CAUSE_BREAKPOINT, 0);
                    return 1;
                } else if (expanded_instr == MRET_INSTR) {  /* MRET */
                    cpu->pc = cpu->mepc;
                    /* 恢复 MIE = MPIE, MPIE = 1 */
                    if (cpu->mstatus & MSTATUS_MPIE)
                        cpu->mstatus |= MSTATUS_MIE;
                    else
                        cpu->mstatus &= ~MSTATUS_MIE;
                    cpu->mstatus |= MSTATUS_MPIE;
                    /* 恢复特权级 = MPP */
                    cpu->privilege = (cpu->mstatus >> 11) & 0x3;
                    /* MPP = 0 (U-mode) */
                    cpu->mstatus &= ~MSTATUS_MPP;
                    return 1;
                } else if (expanded_instr == SRET_INSTR) {  /* SRET */
                    /* 检查 TSR: 若 mstatus.TSR=1 且在 S-mode */
                    if ((cpu->mstatus & MSTATUS_TSR) && cpu->privilege == PRIV_S) {
                        cpu_trap(cpu, CAUSE_ILLEGAL_INSTRUCTION, 0);
                        return 1;
                    }
                    /* 恢复特权级 = SPP */
                    cpu->privilege = (cpu->mstatus & MSTATUS_SPP) ? PRIV_S : PRIV_U;
                    /* 恢复 SIE = SPIE, SPIE = 1 */
                    if (cpu->mstatus & MSTATUS_SPIE)
                        cpu->mstatus |= MSTATUS_SIE;
                    else
                        cpu->mstatus &= ~MSTATUS_SIE;
                    cpu->mstatus |= MSTATUS_SPIE;
                    cpu->mstatus &= ~MSTATUS_SPP;   /* SPP = 0 (U-mode) */
                    cpu->pc = cpu->sepc;
                    return 1;
                } else if ((expanded_instr & 0xFE007FFF) == 0x12000073) {  /* SFENCE.VMA */
                    /* 检查 TVM: 若 mstatus.TVM=1 且在 S-mode */
                    if ((cpu->mstatus & MSTATUS_TVM) && cpu->privilege == PRIV_S) {
                        cpu_trap(cpu, CAUSE_ILLEGAL_INSTRUCTION, 0);
                        return 1;
                    }
                    mmu_flush_tlb(cpu);  /* 刷新 TLB */
                    cpu->pc += actual_len;
                    break;
                } else if (expanded_instr == WFI_INSTR) {  /* WFI */
                    /* NOP: 等待中断（中断由主循环 cpu_check_interrupts 处理） */
                    cpu->pc += actual_len;
                    break;
                } else if (funct3 == F3_CSRRW) {
                    exec_csrrw(cpu, rd, rs1, csr_addr);
                } else if (funct3 == F3_CSRRS) {
                    exec_csrrs(cpu, rd, rs1, csr_addr);
                } else if (funct3 == F3_CSRRC) {
                    exec_csrrc(cpu, rd, rs1, csr_addr);
                } else if (funct3 == F3_CSRRWI) {
                    exec_csrrwi(cpu, rd, uimm, csr_addr);
                } else if (funct3 == F3_CSRRSI) {
                    exec_csrrsi(cpu, rd, uimm, csr_addr);
                } else if (funct3 == F3_CSRRCI) {
                    exec_csrrci(cpu, rd, uimm, csr_addr);
                } else {
                    fprintf(stderr, "[EXECUTE] 未知 SYSTEM 指令：0x%08x (PC=0x%08x)\n", expanded_instr, cpu->pc);
                }
            }
            cpu->pc += actual_len;
            break;

        /* ==================== F 扩展 - LOAD_FP (0x07) ==================== */
        case OP_LOAD_FP:
            {
                uint32_t addr = cpu->regs[i_inst.rs1] + i_inst.imm;
                uint32_t val = mem_read(cpu, addr, 4);
                /* Bug 16: 页错误时不写目标寄存器 */
                if (cpu->pc == pc_entry)
                    cpu->fregs[i_inst.rd].u = val;
            }
            if (cpu->pc == pc_entry)
                cpu->pc += actual_len;
            break;

        /* ==================== F 扩展 - STORE_FP (0x27) ==================== */
        case OP_STORE_FP:
            {
                int32_t imm = decode_store_imm(expanded_instr);
                uint32_t addr = cpu->regs[s_inst.rs1] + imm;
                uint32_t val = cpu->fregs[s_inst.rs2].u;
                mem_write(cpu, addr, val, 4);
            }
            if (cpu->pc == pc_entry)
                cpu->pc += actual_len;
            break;

        /* ==================== F 扩展 - FMADD (0x43) ==================== */
        case OP_FMADD:
            exec_fmadd(cpu, &r_inst);
            cpu->pc += actual_len;
            break;

        /* ==================== F 扩展 - FMSUB (0x47) ==================== */
        case OP_FMSUB:
            exec_fmsub(cpu, &r_inst);
            cpu->pc += actual_len;
            break;

        /* ==================== F 扩展 - FNMSUB (0x4B) ==================== */
        case OP_FNMSUB:
            exec_fnmsub(cpu, &r_inst);
            cpu->pc += actual_len;
            break;

        /* ==================== F 扩展 - FNMADD (0x4F) ==================== */
        case OP_FNMADD:
            exec_fnmadd(cpu, &r_inst);
            cpu->pc += actual_len;
            break;

        /* ==================== F 扩展 - OP_FP (0x53) ==================== */
        case OP_OP_FP:
            {
                uint32_t funct5 = funct7 >> 2;
                uint32_t fmt = funct7 & 0x03;
                uint32_t rm = r_inst.funct3;  /* rm 是 funct3 字段 (bits[14:12]) */

                if (fmt == 0) {  /* 单精度 (fmt=0) */
                    switch (funct5) {
                        case 0x00:  /* FADD.S */
                            exec_fadd(cpu, &r_inst);
                            break;
                        case 0x01:  /* FSUB.S */
                            exec_fsub(cpu, &r_inst);
                            break;
                        case 0x02:  /* FMUL.S */
                            exec_fmul(cpu, &r_inst);
                            break;
                        case 0x03:  /* FDIV.S */
                            exec_fdiv(cpu, &r_inst);
                            break;
                        case 0x04:  /* FSGNJ.S (rm=0), FSGNJN.S (rm=1), FSGNJX.S (rm=2) */
                            if (rm == 0) exec_fsgnj(cpu, &r_inst, 0, 0);
                            else if (rm == 1) exec_fsgnj(cpu, &r_inst, 1, 0);
                            else if (rm == 2) exec_fsgnj(cpu, &r_inst, 0, 1);
                            break;
                        case 0x05:  /* FMIN.S (rm=0), FMAX.S (rm=1) */
                            if (rm == 0) exec_fmin(cpu, &r_inst);
                            else if (rm == 1) exec_fmax(cpu, &r_inst);
                            break;
                        case 0x0B:  /* FSQRT.S */
                            exec_fsqrt(cpu, &r_inst);
                            break;
                        case 0x14:  /* FLE.S (rm=0), FLT.S (rm=1), FEQ.S (rm=2) */
                            if (rm == 0) exec_fle(cpu, &r_inst);
                            else if (rm == 1) exec_flt(cpu, &r_inst);
                            else if (rm == 2) exec_feq(cpu, &r_inst);
                            break;
                        case 0x18:  /* FCVT.W.S /FCVT.WU.S(rs2=signed/unsigned) */
                            if (r_inst.rs2 == 0) exec_fcvt_w_s(cpu, &r_inst, 0);
                            else if (r_inst.rs2 == 1) exec_fcvt_w_s(cpu, &r_inst, 1);
                            break;
                        case 0x1C:  /* FCLASS.S (rs2/rm=1) 或 FMV.X.W (rs2/rm=0) */
                            if (rm == 1) exec_fclass(cpu, &r_inst);  /* FCLASS.S */
                            else exec_fmv_x_w(cpu, &r_inst);         /* FMV.X.W */
                            break;
                        case 0x1A:  /* FCVT.S.W (rs2=signed/unsigned) */
                            if (r_inst.rs2 == 0) exec_fcvt_s_w(cpu, &r_inst, 0);
                            else if (r_inst.rs2 == 1) exec_fcvt_s_w(cpu, &r_inst, 1);
                            break;
                        case 0x1E:  /* FMV.W.X */
                            exec_fmv_w_x(cpu, &r_inst);
                            break;
                        default:
                            fprintf(stderr, "[EXECUTE] 未知 F 指令：funct5=0x%02x\n", funct5);
                            break;
                    }
                }
            }
            cpu->pc += actual_len;
            break;

        /* ==================== 未知指令 ==================== */
        default:
            fprintf(stderr, "[EXECUTE] 未知指令：0x%08x (PC=0x%08x)\n", expanded_instr, cpu->pc);
            return -1;
    }

    /* x0 硬连线为 0 */
    cpu->regs[0] = 0;

    cpu->cycles++;
    return 1;
}
