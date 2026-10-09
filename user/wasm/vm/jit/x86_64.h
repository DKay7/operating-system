#pragma once

#include <inc/types.h>

#include "vm/jit/codebuf.h"

// clang-format off
typedef enum {
    RAX = 0x000,
    RCX = 0x001,
    RDX = 0x002,
    RBX = 0x003,
    RSP = 0x004,
    RBP = 0x005,
    RSI = 0x006,
    RDI = 0x007,
    R8  = 0x008,
    R9  = 0x009,
    R10 = 0x00a,
    R11 = 0x00b,
    R12 = 0x00c,
    R13 = 0x00d,
    R14 = 0x00e,
    R15 = 0x00f,
} Reg;

typedef enum {
    REX_W        = 0x48,
    MOV_R64_IMM  = 0xB8,
    MOV_R64_R64  = 0x89,
    MOV_R64_M64  = 0x8B,
    ADD_RM32_R32 = 0x01,
    ADD_RM64_IMM = 0x81,
    PUSH_R64     = 0x50,
    POP_R64      = 0x58,
    CALL_RM64    = 0xFF,
    RET          = 0xC3,
    TEST_RM32_R32 = 0x85,
    JMP_REL32    = 0xE9,
    JCC_PREFIX   = 0x0F,
    JCC_REL32    = 0x80,
    CMP_RM_R     = 0x39,
    TWO_BYTE     = 0x0F,
    SETCC        = 0x90,
    MOVZX_R_RM8  = 0xB6,
    SUB_RM_R     = 0x29,
    AND_RM_R     = 0x21,
    OR_RM_R      = 0x09,
    XOR_RM_R     = 0x31,
    IMUL_R_RM    = 0xAF,
    SHIFT_RM_CL  = 0xD3,
    DIV_RM       = 0xF7,
    CDQ          = 0x99,
    PREFIX_F3    = 0xF3,
    POPCNT_R_RM  = 0xB8,
    BSF_R_RM     = 0xBC,
    BSR_R_RM     = 0xBD,
    MOVZX_R_RM16 = 0xB7,
    MOVSX_R_RM8  = 0xBE,
    MOVSX_R_RM16 = 0xBF,
    MOVSXD_R_RM32 = 0x63,
    MOV_RM8_R8   = 0x88,
    CMP_R_RM     = 0x3B,
    PREFIX_66    = 0x66,
} x86Op;

// goes to ModRM.reg
typedef enum {
    ADD_IMM_EXT = 0,
    CALL_EXT = 2,
    ROL_EXT  = 0,
    ROR_EXT  = 1,
    SHL_EXT  = 4,
    SHR_EXT  = 5,
    SAR_EXT  = 7,
    DIV_EXT  = 6,
    IDIV_EXT = 7,
} x86OpExt;

typedef enum {
    UNCOND = -1, // just for clarity let it be here
    CC_E    = 0x4,
    CC_NE   = 0x5,
    CC_B    = 0x2,
    CC_AE   = 0x3,
    CC_BE   = 0x6,
    CC_A    = 0x7,
    CC_L    = 0xC,
    CC_GE   = 0xD,
    CC_LE   = 0xE,
    CC_G    = 0xF,
} Cond;
// clang-format on

void x86_mov_r64_imm64(CodeBuf* buf, Reg dst, uint64_t imm);
void x86_mov_r64_r64(CodeBuf* buf, Reg dst, Reg src);
void x86_mov_r64_m64(CodeBuf* buf, Reg dst, Reg base, int32_t disp);
void x86_mov_m64_r64(CodeBuf* buf, Reg base, int32_t disp, Reg src);

// dst = [base + disp] of the given width, zero- (zx) or sign- (sx) extended;
// 32-bit forms zero the upper half of dst
void x86_mov_r32_m32(CodeBuf* buf, Reg dst, Reg base, int32_t disp);
void x86_movzx_r32_m8(CodeBuf* buf, Reg dst, Reg base, int32_t disp);
void x86_movzx_r32_m16(CodeBuf* buf, Reg dst, Reg base, int32_t disp);
void x86_movsx_r32_m8(CodeBuf* buf, Reg dst, Reg base, int32_t disp);
void x86_movsx_r32_m16(CodeBuf* buf, Reg dst, Reg base, int32_t disp);
void x86_movsx_r64_m8(CodeBuf* buf, Reg dst, Reg base, int32_t disp);
void x86_movsx_r64_m16(CodeBuf* buf, Reg dst, Reg base, int32_t disp);
void x86_movsxd_r64_m32(CodeBuf* buf, Reg dst, Reg base, int32_t disp);
// dst = sign-extended src32
void x86_movsxd_r64_r32(CodeBuf* buf, Reg dst, Reg src);

// [base + disp] = low 8 / 16 / 32 bits of src
void x86_mov_m8_r8(CodeBuf* buf, Reg base, int32_t disp, Reg src);
void x86_mov_m16_r16(CodeBuf* buf, Reg base, int32_t disp, Reg src);
void x86_mov_m32_r32(CodeBuf* buf, Reg base, int32_t disp, Reg src);

// flags of a - [base + disp]
void x86_cmp_r64_m64(CodeBuf* buf, Reg a, Reg base, int32_t disp);
void x86_add_r32_r32(CodeBuf* buf, Reg dst, Reg src);
void x86_add_r64_imm32(CodeBuf* buf, Reg dst, int32_t imm);

// dst op= src; 32-bit forms zero the upper half of dst
// clang-format off
void x86_add_r64_r64(CodeBuf* buf, Reg dst, Reg src);
void x86_sub_r32_r32(CodeBuf* buf, Reg dst, Reg src);   void x86_sub_r64_r64(CodeBuf* buf, Reg dst, Reg src);
void x86_and_r32_r32(CodeBuf* buf, Reg dst, Reg src);   void x86_and_r64_r64(CodeBuf* buf, Reg dst, Reg src);
void x86_or_r32_r32(CodeBuf* buf, Reg dst, Reg src);    void x86_or_r64_r64(CodeBuf* buf, Reg dst, Reg src);
void x86_xor_r32_r32(CodeBuf* buf, Reg dst, Reg src);   void x86_xor_r64_r64(CodeBuf* buf, Reg dst, Reg src);
void x86_imul_r32_r32(CodeBuf* buf, Reg dst, Reg src);  void x86_imul_r64_r64(CodeBuf* buf, Reg dst, Reg src);

// dst op= cl; the count is taken modulo 32 / 64 by the CPU, as wasm wants
void x86_shl_r32_cl(CodeBuf* buf, Reg dst);             void x86_shl_r64_cl(CodeBuf* buf, Reg dst);
void x86_shr_r32_cl(CodeBuf* buf, Reg dst);             void x86_shr_r64_cl(CodeBuf* buf, Reg dst);
void x86_sar_r32_cl(CodeBuf* buf, Reg dst);             void x86_sar_r64_cl(CodeBuf* buf, Reg dst);
void x86_rol_r32_cl(CodeBuf* buf, Reg dst);             void x86_rol_r64_cl(CodeBuf* buf, Reg dst);
void x86_ror_r32_cl(CodeBuf* buf, Reg dst);             void x86_ror_r64_cl(CodeBuf* buf, Reg dst);

// edx:eax / src -> eax quotient, edx remainder (rdx:rax for 64)
void x86_div_r32(CodeBuf* buf, Reg src);                void x86_div_r64(CodeBuf* buf, Reg src);
void x86_idiv_r32(CodeBuf* buf, Reg src);               void x86_idiv_r64(CodeBuf* buf, Reg src);
// sign-extend eax into edx:eax / rax into rdx:rax, before idiv
void x86_cdq(CodeBuf* buf);                             void x86_cqo(CodeBuf* buf);

// dst = number of set bits / index of the highest / lowest set bit of src
// (bsr and bsf leave dst undefined and set ZF if src is 0)
void x86_popcnt_r32_r32(CodeBuf* buf, Reg dst, Reg src);    void x86_popcnt_r64_r64(CodeBuf* buf, Reg dst, Reg src);
void x86_bsr_r32_r32(CodeBuf* buf, Reg dst, Reg src);       void x86_bsr_r64_r64(CodeBuf* buf, Reg dst, Reg src);
void x86_bsf_r32_r32(CodeBuf* buf, Reg dst, Reg src);       void x86_bsf_r64_r64(CodeBuf* buf, Reg dst, Reg src);
// clang-format on

void x86_push_r64(CodeBuf* buf, Reg reg);
void x86_pop_r64(CodeBuf* buf, Reg reg);

void x86_call_r64(CodeBuf* buf, Reg target);

void x86_test_r32_r32(CodeBuf* buf, Reg a, Reg b);
void x86_test_r64_r64(CodeBuf* buf, Reg a, Reg b);
// cmp a, b: flags of a - b
void x86_cmp_r32_r32(CodeBuf* buf, Reg a, Reg b);
void x86_cmp_r64_r64(CodeBuf* buf, Reg a, Reg b);
// dst8 = cond ? 1 : 0, the low byte of dst only
void x86_setcc_r8(CodeBuf* buf, Cond cond, Reg dst);
// dst32 = zero-extended low byte of src
void x86_movzx_r32_r8(CodeBuf* buf, Reg dst, Reg src);
size_t x86_jmp(CodeBuf* buf);
size_t x86_jcc(CodeBuf* buf, Cond cond);
void x86_patch_rel32(CodeBuf* buf, size_t at, size_t target);
void x86_ret(CodeBuf* buf);
