#include "common/check.h"
#include "vm/jit/x86_64.h"

#define TODO() CHECK(false, "%s is not implemented\n", __func__)

// http://ref.x86asm.net/coder64.html
// https://wiki.osdev.org/X86-64_Instruction_Encoding#ModR/M_and_SIB_bytes
//
// [REX] [op] [ModRM] [SIB] [offset] [const]
//
// REX = 0100 WRXB
//       W = 64 bit op
//       R = 4th bit of ModRM.reg
//       X = unused for me
//       B = 4th bit of ModRM.rm
//
// ModRM = [mod]:2 [reg]:3 [rm]:3
// ModRM.reg contains only 3 bit of reg, 4th in the REX.r

enum {
    MOD_MEM = 0,    // [rm]
    MOD_DISP8 = 1,  // [rm + disp8]
    MOD_DISP32 = 2, // [rm + disp32]
    MOD_REG = 3,    // rm is a register
};

static void
emit_rex(CodeBuf* buf, bool w64, uint8_t reg, uint8_t rm) {
    uint8_t fixed = 0b0100 << 4; // 0100 ....
    uint8_t w = w64 ? 1 : 0;     // for 64
    uint8_t r = (reg >> 3) & 1;  // 4th bit of reg
    uint8_t x = 0;               //  not used
    uint8_t b = (rm >> 3) & 1;   // 4th bit of ModRM.rm
    emit_u8(buf, fixed | w << 3 | r << 2 | x << 1 | b << 0);
}

static void
emit_rex_if_needed(CodeBuf* buf, uint8_t reg, uint8_t rm) {
    if ((reg | rm) & 0x8) // fucking hate x86
        emit_rex(buf, false, reg, rm);
}

static void
emit_modrm(CodeBuf* buf, uint8_t mod, uint8_t reg, uint8_t rm) {
    uint8_t mod_bits = mod & 0x3;
    uint8_t reg_bits = reg & 0x7;
    uint8_t rm_bits = rm & 0x7;
    emit_u8(buf, mod_bits << 6 | reg_bits << 3 | rm_bits << 0);
}

// [base + disp32] in ModRM.rm, reg in ModRM.reg
static void
emit_mem_operand(CodeBuf* buf, uint8_t reg, Reg base, int32_t disp) {
    emit_modrm(buf, MOD_DISP32, reg, base);
    if ((base & 0x7) == 4)  // rm = 0x100 neededs SIM (rsp, r12)
        emit_u8(buf, 0x24); // i fucking hate x86
    emit_u32(buf, disp);
}

void
x86_mov_r64_imm64(CodeBuf* buf, Reg dst, uint64_t imm) {
    emit_rex(buf, true, 0, dst);
    emit_u8(buf, MOV_R64_IMM + (dst & 0x7));
    emit_u64(buf, imm);
}

// dst in ModRM.rm, src in ModRM.reg
void
x86_mov_r64_r64(CodeBuf* buf, Reg dst, Reg src) {
    emit_rex(buf, true, src, dst);
    emit_u8(buf, MOV_R64_R64);
    emit_modrm(buf, MOD_REG, src, dst);
}

// dst in reg, memory in rm
// mov dst, [base + disp]
void
x86_mov_r64_m64(CodeBuf* buf, Reg dst, Reg base, int32_t disp) {
    emit_rex(buf, true, dst, base);
    emit_u8(buf, MOV_R64_M64);
    emit_mem_operand(buf, dst, base, disp);
}

// src in reg, memory in rm
// mov [base + disp], src
void
x86_mov_m64_r64(CodeBuf* buf, Reg base, int32_t disp, Reg src) {
    emit_rex(buf, true, src, base);
    emit_u8(buf, MOV_R64_R64);
    emit_mem_operand(buf, src, base, disp);
}

void
x86_add_r32_r32(CodeBuf* buf, Reg dst, Reg src) {
    emit_rex_if_needed(buf, src, dst);
    emit_u8(buf, ADD_RM32_R32);
    emit_modrm(buf, MOD_REG, src, dst);
}

void
x86_add_r64_imm32(CodeBuf* buf, Reg dst, int32_t imm) {
    emit_rex(buf, true, ADD_IMM_EXT, dst);
    emit_u8(buf, ADD_RM64_IMM);
    emit_modrm(buf, MOD_REG, ADD_IMM_EXT, dst);
    emit_u32(buf, imm);
}

// no ModRM since always 64, so REX only for r8-r15
void
x86_push_r64(CodeBuf* buf, Reg reg) {
    emit_rex_if_needed(buf, 0, reg);
    emit_u8(buf, PUSH_R64 + (reg & 0x7));
}

void
x86_pop_r64(CodeBuf* buf, Reg reg) {
    emit_rex_if_needed(buf, 0, reg);
    emit_u8(buf, POP_R64 + (reg & 0x7));
}

void
x86_call_r64(CodeBuf* buf, Reg target) {
    emit_rex_if_needed(buf, CALL_EXT, target);
    emit_u8(buf, CALL_RM64);
    emit_modrm(buf, MOD_REG, CALL_EXT, target);
}

void
x86_ret(CodeBuf* buf) {
    emit_u8(buf, RET);
}
