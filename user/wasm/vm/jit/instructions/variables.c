#include "parser/reader.h"
#include "vm/jit/jit.h"
#include "vm/jit/x86_64.h"
#include <stddef.h>
#include <stdint.h>

// push locals[idx] on strck
bool
jit_local_get(JitCtx* jit) {
    uint32_t idx = read_u32_leb(&jit->code);

    // locals is array of uint64, so mult by sizeof
    x86_mov_r64_m64(jit->buf, RAX, R12, idx * sizeof(uint64_t));
    jit_wasm_push(jit, RAX);
    return true;
}

// pop into locals[idx]
bool
jit_local_set(JitCtx* jit) {
    uint32_t idx = read_u32_leb(&jit->code);

    jit_wasm_pop(jit, RAX);

    // locals is array of uint64, so mult by sizeof
    x86_mov_m64_r64(jit->buf, R12, idx * sizeof(uint64_t), RAX);
    return true;
}


bool
jit_local_tee(JitCtx* jit) {
    uint32_t idx = read_u32_leb(&jit->code);

    x86_mov_r64_m64(jit->buf, RAX, RBX, -(int32_t)sizeof(uint64_t));
    x86_mov_m64_r64(jit->buf, R12, idx * sizeof(uint64_t), RAX);
    return true;
}
