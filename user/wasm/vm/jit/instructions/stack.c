#include "vm/jit/jit.h"

// in some cases we have to move height manually
// to do SMART stuff
// mostly, while working with labels
// so, *_raw funcs may be useful there
//
void
jit_wasm_push_raw(JitCtx* jit, Reg reg) {
    x86_mov_m64_r64(jit->buf, RBX, 0, reg);
    x86_add_r64_imm32(jit->buf, RBX, 8);
}

void
jit_wasm_pop_raw(JitCtx* jit, Reg reg) {
    x86_add_r64_imm32(jit->buf, RBX, -8);
    x86_mov_r64_m64(jit->buf, reg, RBX, 0);
}

void
jit_wasm_push(JitCtx* jit, Reg reg) {
    jit_wasm_push_raw(jit, reg);
    jit->height++;
}

void
jit_wasm_pop(JitCtx* jit, Reg reg) {
    jit_wasm_pop_raw(jit, reg);
    jit->height--;
}
