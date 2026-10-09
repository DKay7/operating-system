#include "parser/reader.h"
#include "vm/jit/jit.h"
#include "vm/jit/x86_64.h"

bool
jit_i32_const(JitCtx* jit) {
    uint32_t value = read_i32_leb(&jit->code); // already zext'ed
    x86_mov_r64_imm64(jit->buf, RAX, value);
    jit_wasm_push(jit, RAX);
    return true;
}

#define MATH_OP(width, name, x86_name)                            \
    bool jit_i##width##_##name(JitCtx* jit) {                     \
        jit_wasm_pop(jit, RCX);                                   \
        jit_wasm_pop(jit, RAX);                                   \
        x86_##x86_name##_r##width##_r##width(jit->buf, RAX, RCX); \
        jit_wasm_push(jit, RAX);                                  \
        return true;                                              \
    }

// clang-format off
MATH_OP(32, add, add);    MATH_OP(64, add, add);
MATH_OP(32, sub, sub);    MATH_OP(64, sub, sub);
MATH_OP(32, mul, imul);   MATH_OP(64, mul, imul);
MATH_OP(32, and, and);    MATH_OP(64, and, and);
MATH_OP(32, or,  or);     MATH_OP(64, or,  or);
MATH_OP(32, xor, xor);    MATH_OP(64, xor, xor);
// clang-format on
#undef MATH_OP

static bool
emit_compare(JitCtx* jit, bool w64, Cond cond) {
    jit_wasm_pop(jit, RAX);
    jit_wasm_pop(jit, RCX);

    if (w64)
        x86_cmp_r64_r64(jit->buf, RCX, RAX);
    else
        x86_cmp_r32_r32(jit->buf, RCX, RAX);

    // its actually AL
    x86_setcc_r8(jit->buf, cond, RAX);

    x86_movzx_r32_r8(jit->buf, RAX, RAX);

    jit_wasm_push(jit, RAX);

    return true;
}

static bool
emit_eqz(JitCtx* jit, bool w64) {

    jit_wasm_pop(jit, RAX);

    if (w64)
        x86_test_r64_r64(jit->buf, RAX, RAX);
    else
        x86_test_r32_r32(jit->buf, RAX, RAX);

    x86_setcc_r8(jit->buf, CC_E, RAX);
    x86_movzx_r32_r8(jit->buf, RAX, RAX);
    jit_wasm_push(jit, RAX);

    return true;
}

#define EQZ(width)                         \
    bool jit_i##width##_eqz(JitCtx* jit) { \
        return emit_eqz(jit, width == 64); \
    }

#define CMP(width, name, cond)                       \
    bool jit_i##width##_##name(JitCtx* jit) {        \
        return emit_compare(jit, width == 64, cond); \
    }

// clang-format off
EQZ(32);                     EQZ(64);
CMP(32, eq,   CC_E);         CMP(64, eq,   CC_E);
CMP(32, ne,   CC_NE);        CMP(64, ne,   CC_NE);
CMP(32, lt_s, CC_L);         CMP(64, lt_s, CC_L);
CMP(32, lt_u, CC_B);         CMP(64, lt_u, CC_B);
CMP(32, gt_s, CC_G);         CMP(64, gt_s, CC_G);
CMP(32, gt_u, CC_A);         CMP(64, gt_u, CC_A);
CMP(32, le_s, CC_LE);        CMP(64, le_s, CC_LE);
CMP(32, le_u, CC_BE);        CMP(64, le_u, CC_BE);
CMP(32, ge_s, CC_GE);        CMP(64, ge_s, CC_GE);
CMP(32, ge_u, CC_AE);        CMP(64, ge_u, CC_AE);
// clang-format on
//
#undef EQZ
#undef CMP

bool
jit_i64_const(JitCtx* jit) {
    return false;
}

#define SHIFT(width, name, x86_name)                   \
    bool jit_i##width##_##name(JitCtx* jit) {          \
        jit_wasm_pop(jit, RCX);                        \
        jit_wasm_pop(jit, RAX);                        \
        x86_##x86_name##_r##width##_cl(jit->buf, RAX); \
        jit_wasm_push(jit, RAX);                       \
        return true;                                   \
    }

SHIFT(32, shl, shl);
SHIFT(32, shr_s, sar);
SHIFT(32, shr_u, shr);

SHIFT(64, shl, shl);
SHIFT(64, shr_s, sar);
SHIFT(64, shr_u, shr);

#undef SHIFT

bool
jit_i32_div_u(JitCtx* jit) {
    jit_wasm_pop(jit, RCX);
    jit_wasm_pop(jit, RAX);
    x86_xor_r32_r32(jit->buf, RDX, RDX);
    x86_div_r32(jit->buf, RCX);
    jit_wasm_push(jit, RAX);
    return true;
}

bool
jit_i32_rem_u(JitCtx* jit) {
    jit_wasm_pop(jit, RCX);
    jit_wasm_pop(jit, RAX);
    x86_xor_r32_r32(jit->buf, RDX, RDX);
    x86_div_r32(jit->buf, RCX);
    jit_wasm_push(jit, RDX);
    return true;
}


bool
jit_i32_div_s(JitCtx* jit) {
    jit_wasm_pop(jit, RCX);
    jit_wasm_pop(jit, RAX);
    x86_cdq(jit->buf);
    x86_idiv_r32(jit->buf, RCX);
    jit_wasm_push(jit, RAX);
    return true;
}

bool
jit_i32_rem_s(JitCtx* jit) {
    // have to think about INT_MIN % -1 
    // according to wasm spec
    // which was obviously written by dumbasses
    // we have to implement it like 
    // rhs == -1 ? 0 : lhs % rhs
    //
    // jit_wasm_pop(jit, RCX);
    // jit_wasm_pop(jit, RAX);
    // x86_cdq(jit->buf);
    // x86_idiv_r32(jit->buf, RCX);
    // jit_wasm_push(jit, RDX);
    // return true;
    return false;
}


