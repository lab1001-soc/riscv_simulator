/*
 * decode.c - 译码模块
 * 负责解析指令格式，提取操作码、寄存器、立即数等字段
 * 支持 32 位标准指令和 16 位压缩指令展开
 */

#include "cpu.h"
#include <stdio.h>

/* ==================== 压缩指令立即数解码 ==================== */

/**
 * 压缩寄存器映射：rd'/rs1'/rs2' -> x8-x15
 */
int c_reg_map(int reg) {
    return reg + 8;
}

/**
 * decode_c_addi4spn_imm - 解码 C.ADDI4SPN 立即数
 * nzuimm[5:4:9:8:7:6:2:3] = inst[12:11:10:9:8:7:6:5]
 */
int32_t decode_c_addi4spn_imm(uint16_t instr) {
    uint32_t imm = 0;
    imm |= ((instr >> 5) & 0x1) << 3;
    imm |= ((instr >> 6) & 0x1) << 2;
    imm |= ((instr >> 7) & 0x1) << 6;
    imm |= ((instr >> 8) & 0x1) << 7;
    imm |= ((instr >> 9) & 0x1) << 8;
    imm |= ((instr >> 10) & 0x1) << 9;
    imm |= ((instr >> 11) & 0x1) << 4;
    imm |= ((instr >> 12) & 0x1) << 5;
    return imm;
}

/**
 * decode_c_lw_imm - 解码 C.LW/C.FLW 立即数
 * uimm[6:2] 编码：instr[5,12,11,10,6] → uimm[6,5,4,3,2]
 */
int32_t decode_c_lw_imm(uint16_t instr) {
    uint32_t imm = 0;
    imm |= ((instr >> 12) & 0x1) << 5;  /* instr[12] → uimm[5] */
    imm |= ((instr >> 11) & 0x1) << 4;  /* instr[11] → uimm[4] */
    imm |= ((instr >> 10) & 0x1) << 3;  /* instr[10] → uimm[3] */
    imm |= ((instr >> 6) & 0x1) << 2;   /* instr[6] → uimm[2] */
    imm |= ((instr >> 5) & 0x1) << 6;   /* instr[5] → uimm[6] */
    return imm;
}

/**
 * decode_c_fld_imm - 解码 C.FLD/C.FSD 立即数（双精度，8字节对齐）
 * uimm[7:3] 编码：instr[6:5|12:10] → uimm[7,6,5,4,3]
 * 偏移量 = uimm × 8
 */
int32_t decode_c_fld_imm(uint16_t instr) {
    uint32_t offset = 0;
    offset |= ((instr >> 12) & 0x1) << 5;  /* instr[12] → uimm[5] */
    offset |= ((instr >> 11) & 0x1) << 4;  /* instr[11] → uimm[4] */
    offset |= ((instr >> 10) & 0x1) << 3;  /* instr[10] → uimm[3] */
    offset |= ((instr >>  6) & 0x1) << 7;  /* instr[6]  → uimm[7] */
    offset |= ((instr >>  5) & 0x1) << 6;  /* instr[5]  → uimm[6] */
    return offset;
}

/**
 * decode_c_addi_imm - 解码 C.ADDI 立即数
 */
int32_t decode_c_addi_imm(uint16_t instr) {
    uint32_t imm = 0;
    imm |= ((instr >> 2) & 0x1F);
    imm |= ((instr >> 12) & 0x1) << 5;
    if (imm & 0x20) imm |= 0xFFFFFFC0;
    return imm;
}

/**
 * decode_c_li_imm - 解码 C.LI 立即数
 */
int32_t decode_c_li_imm(uint16_t instr) {
    return decode_c_addi_imm(instr);
}

/**
 * decode_c_j_imm - 解码 C.J/C.JAL 立即数
 * imm[11:1] 编码：{inst[12], inst[8], inst[10], inst[9], inst[6], inst[7], inst[2], inst[11], inst[5], inst[4], inst[3]}
 * 即：imm[11]=inst[12], imm[10]=inst[8], imm[9]=inst[10], imm[8]=inst[9], imm[7]=inst[6], imm[6]=inst[7], imm[5]=inst[2], imm[4]=inst[11], imm[3]=inst[5], imm[2]=inst[4], imm[1]=inst[3]
 * 注意：imm[0] = 0
 */
int32_t decode_c_j_imm(uint16_t instr) {
    uint32_t imm = 0;
    imm |= ((instr >> 12) & 0x1) << 11;  /* inst[12] → imm[11] */
    imm |= ((instr >> 11) & 0x1) << 4;   /* inst[11] → imm[4] */
    imm |= ((instr >> 10) & 0x1) << 9;   /* inst[10] → imm[9] */
    imm |= ((instr >>  9) & 0x1) << 8;    /* inst[9] → imm[8] */
    imm |= ((instr >>  8) & 0x1) << 10;   /* inst[8] → imm[10] */
    imm |= ((instr >>  7) & 0x1) << 6;    /* inst[7] → imm[6] */
    imm |= ((instr >>  6) & 0x1) << 7;    /* inst[6] → imm[7] */
    imm |= ((instr >>  5) & 0x1) << 3;    /* inst[5] → imm[3] */
    imm |= ((instr >>  4) & 0x1) << 2;    /* inst[4] → imm[2] */
    imm |= ((instr >>  3) & 0x1) << 1;    /* inst[3] → imm[1] */
    imm |= ((instr >>  2) & 0x1) << 5;    /* inst[2] → imm[5] */
    return sign_extend(imm, 12);  /* 12位符号扩展 */
}

/**
 * decode_c_b_imm - 解码 C.BEQZ/C.BNEZ 立即数
 * imm[8:1] 编码：instr[12,11,10,6,5,4,3,2] → imm[8,4,3,7,6,2,1,5]
 * 注意：imm[0] = 0
 */
int32_t decode_c_b_imm(uint16_t instr) {
    uint32_t imm = 0;
    imm |= ((instr >> 12) & 0x1) << 8;  /* instr[12] → imm[8] */
    imm |= ((instr >> 11) & 0x1) << 4;  /* instr[11] → imm[4] */
    imm |= ((instr >> 10) & 0x1) << 3;  /* instr[10] → imm[3] */
    imm |= ((instr >> 6) & 0x1) << 7;   /* instr[6] → imm[7] */
    imm |= ((instr >> 5) & 0x1) << 6;   /* instr[5] → imm[6] */
    imm |= ((instr >> 4) & 0x1) << 2;   /* instr[4] → imm[2] */
    imm |= ((instr >> 3) & 0x1) << 1;   /* instr[3] → imm[1] */
    imm |= ((instr >> 2) & 0x1) << 5;   /* instr[2] → imm[5] */
    return sign_extend(imm, 9);
}

/**
 * decode_c_lwsp_imm - 解码 C.LWSP/C.FLWSP 立即数
 * uimm[7:2] 编码：inst[12,6,5,4,2,3] → uimm[5,4,3,2,6,7]
 */
int32_t decode_c_lwsp_imm(uint16_t instr) {
    uint32_t imm = 0;
    imm |= ((instr >> 12) & 0x1) << 5;  /* inst[12] → uimm[5] */
    imm |= ((instr >>  6) & 0x1) << 4;   /* inst[6] → uimm[4] */
    imm |= ((instr >>  5) & 0x1) << 3;   /* inst[5] → uimm[3] */
    imm |= ((instr >>  4) & 0x1) << 2;   /* inst[4] → uimm[2] */
    imm |= ((instr >>  3) & 0x1) << 7;   /* inst[3] → uimm[7] */
    imm |= ((instr >>  2) & 0x1) << 6;   /* inst[2] → uimm[6] */
    return imm;
}

/**
 * decode_c_swsp_imm - 解码 C.SWSP/C.FSWSP 立即数
 * uimm[7:2] 编码：inst[8,7,12,11,10,9] → uimm[7,6,5,4,3,2]
 */
int32_t decode_c_swsp_imm(uint16_t instr) {
    uint32_t imm = 0;
    imm |= ((instr >> 12) & 0x1) << 5;  /* inst[12] → uimm[5] */
    imm |= ((instr >> 11) & 0x1) << 4;  /* inst[11] → uimm[4] */
    imm |= ((instr >> 10) & 0x1) << 3;  /* inst[10] → uimm[3] */
    imm |= ((instr >> 9)  & 0x1) << 2;  /* inst[9] → uimm[2] */
    imm |= ((instr >> 8)  & 0x1) << 7;  /* inst[8] → uimm[7] */
    imm |= ((instr >> 7)  & 0x1) << 6;  /* inst[7] → uimm[6] */
    return imm;
}

/**
 * decode_c_slli_imm - 解码 C.SLLI 立即数
 */
int32_t decode_c_slli_imm(uint16_t instr) {
    uint32_t imm = 0;
    imm |= ((instr >> 2) & 0x1F);
    imm |= ((instr >> 12) & 0x1) << 5;
    return imm;
}

/**
 * decode_c_fldsp_imm - 解码 C.FLDSP 立即数
 * uimm[8:3] 编码：instr[4,3,2,12,6,5] → uimm[8,7,6,5,4,3]
 * 偏移量 = uimm × 8
 */
int32_t decode_c_fldsp_imm(uint16_t instr) {
    uint32_t offset = 0;
    offset |= ((instr >> 12) & 0x1) << 5;
    offset |= ((instr >>  6) & 0x1) << 4;
    offset |= ((instr >>  5) & 0x1) << 3;
    offset |= ((instr >>  4) & 0x1) << 8;
    offset |= ((instr >>  3) & 0x1) << 7;
    offset |= ((instr >>  2) & 0x1) << 6;
    return offset;
}

/**
 * decode_c_fsdsp_imm - 解码 C.FSDSP 立即数
 * uimm[8:3] 编码：instr[12:10] → uimm[5:3], instr[9:7] → uimm[8:6]
 * 偏移量 = uimm × 8
 */
int32_t decode_c_fsdsp_imm(uint16_t instr) {
    uint32_t offset = 0;
    offset |= ((instr >> 10) & 0x7) << 3;  /* instr[12:10] → offset[5:3] */
    offset |= ((instr >>  7) & 0x7) << 6;  /* instr[9:7]   → offset[8:6] */
    return offset;
}

/* ==================== 32 位指令编码辅助 ==================== */

static uint32_t make_i_type(uint32_t opcode, uint32_t rd, uint32_t funct3,
                            uint32_t rs1, int32_t imm) {
    return (opcode) | (rd << 7) | (funct3 << 12) | (rs1 << 15) | ((imm & 0xFFF) << 20);
}

static uint32_t make_r_type(uint32_t opcode, uint32_t rd, uint32_t funct3,
                            uint32_t rs1, uint32_t rs2, uint32_t funct7) {
    return (opcode) | (rd << 7) | (funct3 << 12) | (rs1 << 15) | (rs2 << 20) | (funct7 << 25);
}

static uint32_t make_s_type(uint32_t opcode, uint32_t rs1, uint32_t rs2,
                            uint32_t funct3, int32_t imm) {
    return (opcode) | ((imm & 0x1F) << 7) | (funct3 << 12) | (rs1 << 15) |
           (rs2 << 20) | (((imm >> 5) & 0x7F) << 25);
}

static uint32_t make_j_type(uint32_t opcode, uint32_t rd, int32_t imm) {
    uint32_t j = 0;
    j |= (((imm >> 20) & 1) << 31);
    j |= (((imm >> 1) & 0x3FF) << 21);
    j |= (((imm >> 11) & 1) << 20);
    j |= (((imm >> 12) & 0xFF) << 12);
    return (opcode) | (rd << 7) | j;
}

static uint32_t make_b_type(uint32_t funct3, uint32_t rs1, uint32_t rs2, int32_t imm) {
    uint32_t b = 0;
    b |= (((imm >> 12) & 1) << 31);
    b |= (((imm >> 5) & 0x3F) << 25);
    b |= (((imm >> 1) & 0xF) << 8);
    b |= (((imm >> 11) & 1) << 7);
    return (OP_BRANCH) | (((imm >> 1) & 0xF) << 8) | (funct3 << 12) |
           (rs1 << 15) | (rs2 << 20) | (((imm >> 5) & 0x3F) << 25) | (((imm >> 12) & 1) << 31) | (((imm >> 11) & 1) << 7);
}

/* ==================== 压缩指令展开 ==================== */

/**
 * expand_compressed - 将 16 位压缩指令展开为 32 位等效指令
 * @instr: 16 位压缩指令
 *
 * 返回：等效的 32 位指令，0 表示非法/未实现
 */
uint32_t expand_compressed(uint16_t instr) {
    uint16_t quadrant = instr & 0x3;
    uint16_t funct3 = (instr >> 13) & 0x7;

    /* 寄存器字段 */
    int rd = (instr >> 7) & 0x1F;
    int rs2 = (instr >> 2) & 0x1F;
    int rd_prime = ((instr >> 2) & 0x7) + 8;
    int rs1_prime = ((instr >> 7) & 0x7) + 8;
    int rs2_prime = ((instr >> 2) & 0x7) + 8;

    switch (quadrant) {
        /* ==================== Quadrant 0 ==================== */
        case 0x0:
            switch (funct3) {
                case 0x0:  /* C.ADDI4SPN */
                    if ((instr & 0x1FE0) == 0) return 0;  /* nzuimm=0 非法 */
                    return make_i_type(OP_OP_IMM, rd_prime, 0, 2, decode_c_addi4spn_imm(instr));
                case 0x1:  /* C.FLD (RV32DC) - 浮点加载双精度 */
                    return make_i_type(OP_LOAD_FP, rd_prime, 3, rs1_prime, decode_c_fld_imm(instr));
                case 0x2:  /* C.LW */
                    return make_i_type(OP_LOAD, rd_prime, 2, rs1_prime, decode_c_lw_imm(instr));
                case 0x3:  /* C.FLW (RV32FC) */
                    return make_i_type(OP_LOAD_FP, rd_prime, 2, rs1_prime, decode_c_lw_imm(instr));
                case 0x4:  /* Reserved in RV32 */
                    return 0;
                case 0x5:  /* C.FSD (RV32DC) - 浮点存储双精度 */
                    return make_s_type(OP_STORE_FP, rs1_prime, rs2_prime, 3, decode_c_fld_imm(instr));
                case 0x6:  /* C.SW */
                    return make_s_type(OP_STORE, rs1_prime, rs2_prime, 2, decode_c_lw_imm(instr));
                case 0x7:  /* C.FSW (RV32FC) - 浮点存储单精度 */
                    return make_s_type(OP_STORE_FP, rs1_prime, rs2_prime, 2, decode_c_lw_imm(instr));
            }

        /* ==================== Quadrant 1 ==================== */
        case 0x1:
            switch (funct3) {
                case 0x0:  /* C.ADDI / C.NOP */
                    if (rd == 0 && (instr >> 2) == 0) return 0x00000013;  /* NOP */
                    return make_i_type(OP_OP_IMM, rd, 0, rd, decode_c_addi_imm(instr));
                case 0x1:  /* C.JAL (RV32) */
                    {
                        int32_t offset = decode_c_j_imm(instr);
                        return make_j_type(OP_JAL, 1, offset);
                    }
                case 0x2:  /* C.LI */
                    return make_i_type(OP_OP_IMM, rd, 0, 0, decode_c_li_imm(instr));
                case 0x3:  /* C.ADDI16SP / C.LUI */
                    if (rd == 2) {  /* C.ADDI16SP */
                        /* nzimm[5:0] = inst[12|6|5|4|3], 值 = SignExtend(nzimm) * 16 */
                        uint32_t nzimm = 0;
                        nzimm |= ((instr >> 12) & 0x1) << 9;
                        nzimm |= ((instr >>  6) & 0x1) << 4;
                        nzimm |= ((instr >>  5) & 0x1) << 6;
                        nzimm |= ((instr >>  4) & 0x1) << 8;
                        nzimm |= ((instr >>  3) & 0x1) << 7;
                        nzimm |= ((instr >>  2) & 0x1) << 5;
                        /* nzimm[0] = 0 → bit 4 固定为 0 */
                        if (nzimm & 0x200) nzimm |= 0xFFFFFC00;  /* 符号扩展 bit 9 */
                        return make_i_type(OP_OP_IMM, 2, 0, 2, nzimm);
                    } else {  /* C.LUI */
                        int32_t imm = decode_c_li_imm(instr);
                        if (imm == 0) return 0;
                        /* imm 是 6 位有符号数，左移 12 位放到 LUI 指令的 bits[31:12] */
                        uint32_t imm_field = (uint32_t)(imm << 12);
                        return (OP_LUI) | (rd << 7) | imm_field;
                    }
                case 0x4:  /* C.SRLI / C.SRAI / C.ANDI / C.SUB / C.XOR / C.OR / C.AND */
                    {
                        uint16_t funct2 = (instr >> 10) & 0x3;
                        int rd_c = ((instr >> 7) & 0x7) + 8;
                        int rs2_c = ((instr >> 2) & 0x7) + 8;

                        if (funct2 == 0x0) {  /* C.SRLI */
                            int shamt = decode_c_slli_imm(instr);
                            return make_i_type(OP_OP_IMM, rd_c, 5, rd_c, shamt);
                        } else if (funct2 == 0x1) {  /* C.SRAI */
                            int shamt = decode_c_slli_imm(instr);
                            return make_i_type(OP_OP_IMM, rd_c, 5, rd_c, shamt | 0x400);
                        } else if (funct2 == 0x2) {  /* C.ANDI */
                            int32_t imm = decode_c_li_imm(instr);
                            return make_i_type(OP_OP_IMM, rd_c, 7, rd_c, imm);
                        } else {  /* funct2 == 0x3: C.SUB / C.XOR / C.OR / C.AND */
                            /* bit12=1 是 RV64 专用编码 (C.SUBW / C.ADDW)，RV32 中为非法 */
                            if ((instr >> 12) & 0x1) return 0;
                            uint16_t funct6_5 = (instr >> 5) & 0x3;  /* bits[6:5] determine operation */
                            if (funct6_5 == 0x0) return make_r_type(OP_OP, rd_c, 0, rd_c, rs2_c, 0x20);  /* SUB */
                            if (funct6_5 == 0x1) return make_r_type(OP_OP, rd_c, 4, rd_c, rs2_c, 0);    /* XOR */
                            if (funct6_5 == 0x2) return make_r_type(OP_OP, rd_c, 6, rd_c, rs2_c, 0);    /* OR */
                            return make_r_type(OP_OP, rd_c, 7, rd_c, rs2_c, 0);  /* AND */
                        }
                    }
                case 0x5:  /* C.J - 无条件跳转 */
                    return make_j_type(OP_JAL, 0, decode_c_j_imm(instr));
                case 0x6:  /* C.BEQZ - 等于零分支 */
                    {
                        int rs_c = ((instr >> 7) & 0x7) + 8;
                        return make_b_type(0, rs_c, 0, decode_c_b_imm(instr));
                    }
                case 0x7:  /* C.BNEZ - 不等于零分支 */
                    {
                        int rs_c = ((instr >> 7) & 0x7) + 8;
                        return make_b_type(1, rs_c, 0, decode_c_b_imm(instr));
                    }
                default:
                    return 0;
            }

        /* ==================== Quadrant 2 ==================== */
        case 0x2:
            switch (funct3) {
                case 0x0:  /* C.SLLI */
                    {
                        int shamt = decode_c_slli_imm(instr);
                        if (rd == 0) return 0;
                        return make_i_type(OP_OP_IMM, rd, 1, rd, shamt);
                    }
                case 0x1:  /* C.FLDSP (RV32DC/RV64DC) */
                    return make_i_type(OP_LOAD_FP, rd, 3, 2, decode_c_fldsp_imm(instr));
                case 0x2:  /* C.LWSP */
                    return make_i_type(OP_LOAD, rd, 2, 2, decode_c_lwsp_imm(instr));
                case 0x3:  /* C.FLWSP (RV32FC) */
                    return make_i_type(OP_LOAD_FP, rd, 2, 2, decode_c_lwsp_imm(instr));
                case 0x4:  /* C.JR / C.MV / C.ADD / C.EBREAK / C.JALR */
                    if ((instr >> 12) & 0x1) {  /* bit12=1: C.ADD / C.EBREAK / C.JALR */
                        if (rs2 == 0) {  /* rs2=0: C.EBREAK or C.JALR */
                            if (rd == 0) {  /* C.EBREAK: rd=0, rs2=0, bit12=1 */
                                return 0x00100073;
                            } else {  /* C.JALR: rd!=0, rs2=0, bit12=1 */
                                return make_i_type(OP_JALR, 1, 0, rd, 0);
                            }
                        } else {  /* rs2!=0: C.ADD */
                            if (rd == 0) return 0;  /* illegal: C.ADD requires rd!=0 */
                            return make_r_type(OP_OP, rd, 0, rd, rs2, 0);
                        }
                    } else {  /* bit12=0: C.JR or C.MV */
                        if (rs2 == 0) {  /* C.JR: rs2=0, bit12=0 */
                            if (rd == 0) return 0;  /* illegal: C.JR requires rd!=0 */
                            return make_i_type(OP_JALR, 0, 0, rd, 0);
                        } else {  /* C.MV: rs2!=0, bit12=0 */
                            if (rd == 0) return 0;  /* illegal: C.MV requires rd!=0 */
                            return make_r_type(OP_OP, rd, 0, 0, rs2, 0);
                        }
                    }
                case 0x5:  /* C.FSDSP (RV32DC/RV64DC) */
                    return make_s_type(OP_STORE_FP, 2, rs2, 3, decode_c_fsdsp_imm(instr));
                case 0x6:  /* C.SWSP */
                    return make_s_type(OP_STORE, 2, rs2, 2, decode_c_swsp_imm(instr));
                case 0x7:  /* C.FSWSP (RV32FC) */
                    return make_s_type(OP_STORE_FP, 2, rs2, 2, decode_c_swsp_imm(instr));
                default:
                    return 0;
            }

        /* ==================== Quadrant 3 (32-bit) ==================== */
        case 0x3:
            return 0;  /* 应该不会被调用 */
    }

    return 0;
}

/* ==================== 标准指令解码 ==================== */

/**
 * cpu_decode - 指令译码
 */
int cpu_decode(CPU *cpu, uint32_t instr, RType *r, IType *i, SType *s, R4Type *r4) {
    (void)cpu;

    /* R 型 */
    r->opcode = instr & 0x7F;
    r->rd     = (instr >> 7) & 0x1F;
    r->funct3 = (instr >> 12) & 0x07;
    r->rs1    = (instr >> 15) & 0x1F;
    r->rs2    = (instr >> 20) & 0x1F;
    r->funct7 = (instr >> 25) & 0x7F;

    /* I 型 */
    i->opcode = instr & 0x7F;
    i->rd     = (instr >> 7) & 0x1F;
    i->funct3 = (instr >> 12) & 0x07;
    i->rs1    = (instr >> 15) & 0x1F;
    i->imm    = sign_extend(instr >> 20, 12);

    /* S 型 */
    s->opcode   = instr & 0x7F;
    s->imm_low  = (instr >> 7) & 0x1F;
    s->funct3   = (instr >> 12) & 0x07;
    s->rs1      = (instr >> 15) & 0x1F;
    s->rs2      = (instr >> 20) & 0x1F;
    s->imm_high = (instr >> 25) & 0x7F;

    /* R4 型 */
    if (r4) {
        r4->opcode = instr & 0x7F;
        r4->rd     = (instr >> 7) & 0x1F;
        r4->rm     = (instr >> 12) & 0x07;
        r4->rs1    = (instr >> 15) & 0x1F;
        r4->rs2    = (instr >> 20) & 0x1F;
        r4->fmt    = (instr >> 25) & 0x03;
        r4->rs3    = (instr >> 27) & 0x1F;
    }

#ifdef DEBUG_DECODE
    printf("[DECODE] INSTR=0x%08x, opcode=0x%02x, rd=%d, rs1=%d, rs2=%d\n",
           instr, r->opcode, r->rd, r->rs1, r->rs2);
#endif

    return 0;
}

/**
 * decode_branch_imm - 解析 B 型指令立即数
 */
int32_t decode_branch_imm(uint32_t instr) {
    uint32_t imm = 0;
    imm |= ((instr >> 31) & 0x01) << 12;
    imm |= ((instr >> 7) & 0x01) << 11;
    imm |= ((instr >> 25) & 0x3F) << 5;
    imm |= ((instr >> 8) & 0x0F) << 1;
    return sign_extend(imm, 13);
}

/**
 * decode_jal_imm - 解析 J 型指令立即数
 */
int32_t decode_jal_imm(uint32_t instr) {
    uint32_t imm = 0;
    imm |= ((instr >> 31) & 0x01) << 20;
    imm |= ((instr >> 21) & 0x3FF) << 1;
    imm |= ((instr >> 20) & 0x01) << 11;
    imm |= ((instr >> 12) & 0xFF) << 12;
    return sign_extend(imm, 21);
}

/**
 * decode_store_imm - 解析 S 型指令立即数
 */
int32_t decode_store_imm(uint32_t instr) {
    uint32_t imm = 0;
    imm |= ((instr >> 25) & 0x7F) << 5;
    imm |= ((instr >> 7) & 0x1F);
    return sign_extend(imm, 12);
}