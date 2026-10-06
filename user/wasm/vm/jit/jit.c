#include <inc/lib.h>

#include "common/check.h"
#include "common/opcodes.h"
#include "vm/jit/jit.h"
#include "vm/jit/codebuf.h"
#include "vm/jit/x86_64.h"

static void jitctx_init(JitCtx* jit, CodeBuf* buf, VM* vm, uint64_t func_idx);
static int64_t jit_compile_func(CodeBuf* buf, VM* vm, uint32_t func_idx);

void
jit_compile_module(VM* vm) {
    Module* module = vm->module;

    CodeBuf* buf = balloc(1, sizeof(CodeBuf));
    CHECK(buf, "out of memory\n");
    codebuf_init(buf);

    int64_t* offsets = balloc(module->nfuncs, sizeof(int64_t));
    CHECK(offsets, "out of memory\n");

    for (uint32_t idx = 0; idx < module->nfuncs; ++idx)
        offsets[idx] = jit_compile_func(buf, vm, idx);

    codebuf_finalize(buf);

    vm->jit_code = balloc(module->nfuncs, sizeof(void*));
    CHECK(vm->jit_code, "out of memory\n");
    for (uint32_t idx = 0; idx < module->nfuncs; ++idx)
        vm->jit_code[idx] = offsets[idx] < 0 ? NULL : buf->code + offsets[idx];

    free(offsets);
}

static int64_t
jit_compile_func(CodeBuf* buf, VM* vm, uint32_t func_idx) {
    JitCtx jit;
    jitctx_init(&jit, buf, vm, func_idx);

    size_t start = buf->len;
    jit_emit_prologue(&jit);

    while (reader_remaining(&jit.code) > 0) {
        uint8_t opcode = read_u8(&jit.code);
        bool supported = false;

        switch (opcode) {
#define OP_DEF(op, val, name, ...)    \
    case op:                          \
        supported = jit_##name(&jit); \
        break;
#include "common/opcodes.def"
#undef OP_DEF
        }

        if (!supported) {
            // drop what was emitted if some instructions are unsupported
            buf->len = start;
            return -1;
        }
    }

    return start;
}

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
void
jit_emit_prologue(JitCtx* jit) {
    CodeBuf* buf = jit->buf;

    // callee saved:
    x86_push_r64(buf, RBX);
    x86_push_r64(buf, R12);
    x86_push_r64(buf, R13);

    // arguments to the fixed registers
    x86_mov_r64_r64(buf, R13, RDI);
    x86_mov_r64_r64(buf, RBX, RSI);
    x86_mov_r64_r64(buf, R12, RDX);
}


void
jit_emit_epilogue(JitCtx* jit) {
    CodeBuf* buf = jit->buf;

    x86_mov_r64_r64(buf, RAX, RBX);

    x86_pop_r64(buf, R13);
    x86_pop_r64(buf, R12);
    x86_pop_r64(buf, RBX);
    x86_ret(buf);
}


static void
jitctx_init(JitCtx* jit, CodeBuf* buf, VM* vm, uint64_t func_idx) {
    Func* func = &vm->module->funcs[func_idx];
    jit->buf = buf;
    jit->vm = vm;
    jit->func = func;

    reader_init(&jit->code, func->code, func->code_size);
}
