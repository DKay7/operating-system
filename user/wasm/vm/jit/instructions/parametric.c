#include "vm/jit/jit.h"
#include "vm/jit/x86_64.h"

bool
jit_drop(JitCtx* jit) {
    jit_wasm_pop(jit, RAX);
    return true;
}

bool
jit_select(JitCtx* jit) {
    jit_wasm_pop(jit, RAX); // cond
    jit_wasm_pop(jit, RCX); // if false
    jit_wasm_pop(jit, RDX); // if true

    x86_test_r32_r32(jit->buf, RAX, RAX);
    // 
    // may be done via cmov but im done with adding x86 opcodes
    size_t keep_true = x86_jcc(jit->buf, CC_NE);
    x86_mov_r64_r64(jit->buf, RDX, RCX);
    x86_patch_rel32(jit->buf, keep_true, jit->buf->len);

    jit_wasm_push(jit, RDX);
    return true;
}
