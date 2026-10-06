#include <inc/lib.h>
#include <stddef.h>

#include "parser/reader.h"
#include "vm/jit/jit.h"
#include "vm/jit/x86_64.h"
#include "vm/vm.h"

// Calling convention:
// Args:
//          rdi = ctx
//          rsi = sp
//          rdx = locasl
//
// Return:
//          rax = stack pointer
//
// While running:
//          rbx = stack pointer
//          r12 = locals
//          r13 = ctx
//          rax = free to use
//          rcx = free to use
//          rdx = free to use
//
// Savings:
//          rbx, r12, r13 = callee saved
//          rax, rcx, rdx = caller saved
//


static void
jit_wasm_push(JitCtx* jit, Reg reg) {
    x86_mov_m64_r64(jit->buf, RBX, 0, reg);
    x86_add_r64_imm32(jit->buf, RBX, 8);
}

static void
jit_wasm_pop(JitCtx* jit, Reg reg) {
    x86_add_r64_imm32(jit->buf, RBX, -8);
    x86_mov_r64_m64(jit->buf, reg, RBX, 0);
}

bool
jit_i32_const(JitCtx* jit) {
    uint32_t value = read_i32_leb(&jit->code); // already zext'ed
    x86_mov_r64_imm64(jit->buf, RAX, value);
    x86_mov_m64_r64(jit->buf, RBX, 0, RAX);
    x86_add_r64_imm32(jit->buf, RBX, 8);
    return true;
}

// push locals[idx] on strck
bool
jit_local_get(JitCtx* jit) {
    uint32_t idx = read_i32_leb(&jit->code);

    // locals is array of uint64, so mult by sizeof
    x86_mov_r64_m64(jit->buf, RAX, R12, idx * sizeof(uint64_t));
    jit_wasm_push(jit, RAX);

    return true;
}

// pop into locals[idx]
bool
jit_local_set(JitCtx* jit) {
    uint32_t idx = read_i32_leb(&jit->code);

    jit_wasm_pop(jit, RAX);

    // locals is array of uint64, so mult by sizeof
    x86_mov_m64_r64(jit->buf, R12, idx * sizeof(uint64_t), RAX);


    return true;
}

bool
jit_i32_add(JitCtx* jit) {
    jit_wasm_pop(jit, RAX);
    jit_wasm_pop(jit, RCX);
    x86_add_r32_r32(jit->buf, RAX, RCX);
    jit_wasm_push(jit, RAX);
    return true;
}

bool
jit_end(JitCtx* jit) {
    // if end is the last instruction
    if (reader_remaining(&jit->code) == 0)
        jit_emit_epilogue(jit);

    return true;
}

//  have to call interpreter
bool
jit_call(JitCtx* jit) {
    x86_mov_r64_imm64(jit->buf, RAX, (uint64_t)&jit->vm->stack.top);
    
    // update stack with jit's one
    x86_mov_m64_r64(jit->buf, RAX, 0, RBX);

    // pass VM *vm as first arg to interpreter
    x86_mov_r64_m64(jit->buf, RDI, R13, offsetof(CallCtx, vm));

    x86_mov_r64_imm64(jit->buf, RAX, (uint64_t)&interp_call);
    x86_call_r64(jit->buf, RAX);

    return true;
}

