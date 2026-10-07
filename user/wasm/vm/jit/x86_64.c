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

// REX.W for 64-bit operands; 32-bit ones need REX only for r8-r15
static void
emit_rex_w(CodeBuf* buf, bool w64, uint8_t reg, uint8_t rm) {
    if (w64)
        emit_rex(buf, true, reg, rm);
    else
        emit_rex_if_needed(buf, reg, rm);
}

// [REX] op ModRM(mod 11, reg, rm)
static void
emit_op_rr(CodeBuf* buf, bool w64, uint8_t opcode, uint8_t reg, uint8_t rm) {
    emit_rex_w(buf, w64, reg, rm);
    emit_u8(buf, opcode);
    emit_modrm(buf, MOD_REG, reg, rm);
}

// [REX] 0F op ModRM(mod 11, reg, rm)
static void
emit_op2_rr(CodeBuf* buf, bool w64, uint8_t opcode, uint8_t reg, uint8_t rm) {
    emit_rex_w(buf, w64, reg, rm);
    emit_u8(buf, TWO_BYTE);
    emit_u8(buf, opcode);
    emit_modrm(buf, MOD_REG, reg, rm);
}

void
x86_mov_r64_imm64(CodeBuf* buf, Reg dst, uint64_t imm) {
    emit_rex_w(buf, true, 0, dst);
    emit_u8(buf, MOV_R64_IMM + (dst & 0x7));
    emit_u64(buf, imm);
}

// dst in ModRM.rm, src in ModRM.reg
void
x86_mov_r64_r64(CodeBuf* buf, Reg dst, Reg src) {
    emit_op_rr(buf, true, MOV_R64_R64, src, dst);
}

// dst in reg, memory in rm
// mov dst, [base + disp]
void
x86_mov_r64_m64(CodeBuf* buf, Reg dst, Reg base, int32_t disp) {
    emit_rex_w(buf, true, dst, base);
    emit_u8(buf, MOV_R64_M64);
    emit_mem_operand(buf, dst, base, disp);
}

// src in reg, memory in rm
// mov [base + disp], src
void
x86_mov_m64_r64(CodeBuf* buf, Reg base, int32_t disp, Reg src) {
    emit_rex_w(buf, true, src, base);
    emit_u8(buf, MOV_R64_R64);
    emit_mem_operand(buf, src, base, disp);
}

void
x86_add_r64_imm32(CodeBuf* buf, Reg dst, int32_t imm) {
    emit_op_rr(buf, true, ADD_RM64_IMM, ADD_IMM_EXT, dst);
    emit_u32(buf, imm);
}

// no ModRM since always 64, so REX only for r8-r15
void
x86_push_r64(CodeBuf* buf, Reg reg) {
    emit_rex_w(buf, false, 0, reg);
    emit_u8(buf, PUSH_R64 + (reg & 0x7));
}

void
x86_pop_r64(CodeBuf* buf, Reg reg) {
    emit_rex_w(buf, false, 0, reg);
    emit_u8(buf, POP_R64 + (reg & 0x7));
}

void
x86_call_r64(CodeBuf* buf, Reg target) {
    emit_op_rr(buf, false, CALL_RM64, CALL_EXT, target);
}

void
x86_ret(CodeBuf* buf) {
    emit_u8(buf, RET);
}

// byte regs 4-7 has different meaning
// based on REX, so write it down...
void
x86_setcc_r8(CodeBuf* buf, Cond cond, Reg dst) {
    if (dst >= RSP)
        emit_rex(buf, false, 0, dst);
    emit_u8(buf, TWO_BYTE);
    emit_u8(buf, SETCC + cond);
    emit_modrm(buf, MOD_REG, 0, dst);
}

void
x86_movzx_r32_r8(CodeBuf* buf, Reg dst, Reg src) {
    if (src >= RSP || dst >= R8)
        emit_rex(buf, false, dst, src); // hatred to x86 grows fucking EXPONENTIALLY
    emit_u8(buf, TWO_BYTE);
    emit_u8(buf, MOVZX_R_RM8);
    emit_modrm(buf, MOD_REG, dst, src);
}

size_t
x86_jmp(CodeBuf* buf) {
    emit_u8(buf, JMP_REL32);
    size_t at = buf->len;
    emit_u32(buf, 0);
    return at;
}

size_t
x86_jcc(CodeBuf* buf, Cond cond) {
    emit_u8(buf, JCC_PREFIX);
    emit_u8(buf, JCC_REL32 + cond);
    size_t at = buf->len;
    emit_u32(buf, 0);
    return at;
}

void
x86_patch_rel32(CodeBuf* buf, size_t at, size_t target) {
    int32_t rel = (int64_t)target - (int64_t)(at + 4);
    for (int idx = 0; idx < 4; ++idx)
        buf->staging[at + idx] = (uint32_t)rel >> (8 * idx);
}

// "OP r/m, r": dst in rm, src in reg
#define ALU_RR(name, w64, bits, opcode)                                   \
    void x86_##name##_r##bits##_r##bits(CodeBuf* buf, Reg dst, Reg src) { \
        emit_op_rr(buf, w64, opcode, src, dst);                           \
    }

// "OP r, r/m": dst in reg, src in rm
#define ALU2_RR(name, w64, bits, opcode)                                  \
    void x86_##name##_r##bits##_r##bits(CodeBuf* buf, Reg dst, Reg src) { \
        emit_op2_rr(buf, w64, opcode, dst, src);                          \
    }

#define SHIFT_CL(name, w64, bits, ext)                      \
    void x86_##name##_r##bits##_cl(CodeBuf* buf, Reg dst) { \
        emit_op_rr(buf, w64, SHIFT_RM_CL, ext, dst);        \
    }

#define DIV(name, w64, bits, ext)                      \
    void x86_##name##_r##bits(CodeBuf* buf, Reg src) { \
        emit_op_rr(buf, w64, DIV_RM, ext, src);        \
    }

// clang-format off
ALU_RR(add, false, 32, ADD_RM32_R32);    ALU_RR(add, true, 64, ADD_RM32_R32);
ALU_RR(sub, false, 32, SUB_RM_R);        ALU_RR(sub, true, 64, SUB_RM_R);
ALU_RR(and, false, 32, AND_RM_R);        ALU_RR(and, true, 64, AND_RM_R);
ALU_RR(or, false, 32, OR_RM_R);          ALU_RR(or, true, 64, OR_RM_R);
ALU_RR(xor, false, 32, XOR_RM_R);        ALU_RR(xor, true, 64, XOR_RM_R);
ALU_RR(cmp, false, 32, CMP_RM_R);        ALU_RR(cmp, true, 64, CMP_RM_R);
ALU_RR(test, false, 32, TEST_RM32_R32);  ALU_RR(test, true, 64, TEST_RM32_R32);
ALU2_RR(imul, false, 32, IMUL_R_RM);     ALU2_RR(imul, true, 64, IMUL_R_RM);
ALU2_RR(bsr, false, 32, BSR_R_RM);       ALU2_RR(bsr, true, 64, BSR_R_RM);
ALU2_RR(bsf, false, 32, BSF_R_RM);       ALU2_RR(bsf, true, 64, BSF_R_RM);

SHIFT_CL(shl, false, 32, SHL_EXT);       SHIFT_CL(shl, true, 64, SHL_EXT);
SHIFT_CL(shr, false, 32, SHR_EXT);       SHIFT_CL(shr, true, 64, SHR_EXT);
SHIFT_CL(sar, false, 32, SAR_EXT);       SHIFT_CL(sar, true, 64, SAR_EXT);
SHIFT_CL(rol, false, 32, ROL_EXT);       SHIFT_CL(rol, true, 64, ROL_EXT);
SHIFT_CL(ror, false, 32, ROR_EXT);       SHIFT_CL(ror, true, 64, ROR_EXT);

DIV(div, false, 32, DIV_EXT);            DIV(div, true, 64, DIV_EXT);
DIV(idiv, false, 32, IDIV_EXT);          DIV(idiv, true, 64, IDIV_EXT);
// clang-format on

#undef ALU_RR
#undef ALU2_RR
#undef SHIFT_CL
#undef DIV

void
x86_cdq(CodeBuf* buf) {
    emit_u8(buf, CDQ);
}

void
x86_cqo(CodeBuf* buf) {
    emit_rex_w(buf, true, 0, 0);
    emit_u8(buf, CDQ);
}

// F3 goes before REX
static void
emit_popcnt(CodeBuf* buf, bool w64, Reg dst, Reg src) {
    emit_u8(buf, PREFIX_F3);
    emit_op2_rr(buf, w64, POPCNT_R_RM, dst, src);
}

void
x86_popcnt_r32_r32(CodeBuf* buf, Reg dst, Reg src) {
    emit_popcnt(buf, false, dst, src);
}

void
x86_popcnt_r64_r64(CodeBuf* buf, Reg dst, Reg src) {
    emit_popcnt(buf, true, dst, src);
}
