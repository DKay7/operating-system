#include <stddef.h>

#include "common/check.h"
#include "parser/reader.h"
#include "vm/interp_call.h"
#include "vm/jit/jit.h"
#include "vm/jit/x86_64.h"
#include "vm/vm.h"

static uint64_t*
jit_memory_trap(CallCtx* ctx, uint64_t* sp, uint64_t opcode_offset) {
    jit_set_stack_top(ctx, sp);
    ctx->pc = opcode_offset;
    callctx_dump(ctx);
    CHECK(false, "out of bounds memory access\n");
    return sp;
}

static void
jit_effective_addr(JitCtx* jit, int32_t size) {
    size_t opcode_offset = jit->code.offset - 1;
    read_u32_leb(&jit->code);
    uint32_t offset = read_u32_leb(&jit->code);

    jit_wasm_pop(jit, RAX);
    x86_mov_r64_imm64(jit->buf, RCX, (uint64_t)offset + size);
    x86_add_r64_r64(jit->buf, RAX, RCX);

    x86_mov_r64_m64(jit->buf, RCX, R13, offsetof(CallCtx, vm));
    x86_cmp_r64_m64(jit->buf, RAX, RCX, offsetof(VM, mem_size));
    size_t in_bounds = x86_jcc(jit->buf, CC_BE);
    jit_emit_runtime_call(jit, jit_memory_trap, opcode_offset);
    x86_patch_rel32(jit->buf, in_bounds, jit->buf->len);

    x86_mov_r64_m64(jit->buf, RCX, RCX, offsetof(VM, memory));
    x86_add_r64_r64(jit->buf, RAX, RCX);
}

#define LOAD(name, size, x86_load)                    \
    bool jit_##name(JitCtx* jit) {                    \
        jit_effective_addr(jit, size);                \
        x86_load(jit->buf, RAX, RAX, -(int32_t)size); \
        jit_wasm_push(jit, RAX);                      \
        return true;                                  \
    }

// clang-format off
LOAD(i32_load,     4, x86_mov_r32_m32);
LOAD(i64_load,     8, x86_mov_r64_m64);

LOAD(i32_load8_s,  1, x86_movsx_r32_m8);
LOAD(i64_load8_s,  1, x86_movsx_r64_m8);

LOAD(i32_load8_u,  1, x86_movzx_r32_m8);
LOAD(i64_load8_u,  1, x86_movzx_r32_m8);

LOAD(i32_load16_s, 2, x86_movsx_r32_m16);
LOAD(i64_load16_s, 2, x86_movsx_r64_m16);

LOAD(i32_load16_u, 2, x86_movzx_r32_m16);
LOAD(i64_load16_u, 2, x86_movzx_r32_m16);

LOAD(i64_load32_s, 4, x86_movsxd_r64_m32);
LOAD(i64_load32_u, 4, x86_mov_r32_m32);
// clang-format on
#undef LOAD

#define STORE(name, size, x86_store)                   \
    bool jit_##name(JitCtx* jit) {                     \
        jit_wasm_pop(jit, RDX);                        \
        jit_effective_addr(jit, size);                 \
        x86_store(jit->buf, RAX, -(int32_t)size, RDX); \
        return true;                                   \
    }

// clang-format off
STORE(i32_store,   4, x86_mov_m32_r32);
STORE(i64_store,   8, x86_mov_m64_r64);

STORE(i32_store8,  1, x86_mov_m8_r8);
STORE(i64_store8,  1, x86_mov_m8_r8);

STORE(i32_store16, 2, x86_mov_m16_r16);
STORE(i64_store16, 2, x86_mov_m16_r16);

STORE(i64_store32, 4, x86_mov_m32_r32);
// clang-format on
#undef STORE
