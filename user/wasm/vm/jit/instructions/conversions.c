#include "vm/jit/jit.h"
#include "vm/jit/x86_64.h"

bool
jit_i32_wrap_i64(JitCtx* jit) {
    jit_wasm_pop(jit, RAX);
    x86_and_r32_r32(jit->buf, RAX, RAX);
    jit_wasm_push(jit, RAX);
    return true;
}

bool
jit_i64_extend_i32_s(JitCtx* jit) {
    jit_wasm_pop(jit, RAX);
    x86_movsxd_r64_r32(jit->buf, RAX, RAX);
    jit_wasm_push(jit, RAX);
    return true;
}

bool
jit_i64_extend_i32_u(JitCtx* jit) {
    return true;
}
