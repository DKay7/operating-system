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
    MOV_R64_IMM  = 0xB8, // B8+r:   MOV r64, imm64
    MOV_R64_R64  = 0x89, // 89 /r:  MOV r/m64, r64 (reg-reg and store)
    MOV_R64_M64  = 0x8B, // 8B /r:  MOV r64, r/m64 (load)
    ADD_RM32_R32 = 0x01, // 01 /r:  ADD r/m32, r32
    ADD_RM64_IMM = 0x81, // 81 /0:  ADD r/m64, imm32
    PUSH_R64     = 0x50, // 50+r:   PUSH r64
    POP_R64      = 0x58, // 58+r:   POP r64
    CALL_RM64    = 0xFF, // FF /2:  CALL r/m64
    RET          = 0xC3, // C3:     RET
} x86Op;

// /digit opcode extensions, they go to ModRM.reg
enum {
    ADD_IMM_EXT = 0, // 81 /0
    CALL_EXT = 2,    // FF /2
};
// clang-format on

// mov dst, imm64
void x86_mov_r64_imm64(CodeBuf* buf, Reg dst, uint64_t imm);
// mov dst, src
void x86_mov_r64_r64(CodeBuf* buf, Reg dst, Reg src);
// mov dst, [base + disp]
void x86_mov_r64_m64(CodeBuf* buf, Reg dst, Reg base, int32_t disp);
// mov [base + disp], src
void x86_mov_m64_r64(CodeBuf* buf, Reg base, int32_t disp, Reg src);
// add dst, src
void x86_add_r32_r32(CodeBuf* buf, Reg dst, Reg src);
// add dst, imm32
void x86_add_r64_imm32(CodeBuf* buf, Reg dst, int32_t imm);

void x86_push_r64(CodeBuf* buf, Reg reg);
void x86_pop_r64(CodeBuf* buf, Reg reg);

void x86_call_r64(CodeBuf* buf, Reg target);
void x86_ret(CodeBuf* buf);
