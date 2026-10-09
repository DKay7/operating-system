#include <inc/lib.h>
#include <stddef.h>

#include "common/check.h"
#include "common/opcodes.h"
#include "vm/interp_call.h"
#include "vm/jit/jit.h"
#include "vm/jit/codebuf.h"
#include "vm/jit/x86_64.h"
#include "vm/vm.h"

static void jitctx_init(JitCtx* jit, CodeBuf* buf, VM* vm, uint64_t func_idx);
static int64_t jit_compile_func(CodeBuf* buf, VM* vm, uint32_t func_idx);
static bool jit_compile_op(JitCtx* jit, uint8_t opcode);
static bool is_control(uint8_t opcode);
static void jit_emit_fallback(JitCtx* jit, uint8_t opcode, size_t opcode_offset);

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
    if (vm->module->funcs[func_idx].imported)
        return -1;

    JitCtx jit;
    jitctx_init(&jit, buf, vm, func_idx);

    size_t start = buf->len;
    jit_emit_prologue(&jit);

    while (reader_remaining(&jit.code) > 0) {
        size_t opcode_offset = jit.code.offset;
        jit.label_offsets[opcode_offset] = buf->len;
        uint8_t opcode = read_u8(&jit.code);

        if (jit_compile_op(&jit, opcode))
            continue;

        if (is_control(opcode)) {
            buf->len = start;
            return -1;
        }
        jit_emit_fallback(&jit, opcode, opcode_offset);
    }

    for (uint32_t idx = 0; idx < jit.nfixups; ++idx)
        x86_patch_rel32(buf, jit.fixups[idx].at, jit.label_offsets[jit.fixups[idx].target]);

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

    x86_mov_r64_r64(buf, R13, RDI); // ctx
    x86_mov_r64_r64(buf, RBX, RSI); // stack
    x86_mov_r64_r64(buf, R12, RDX); // locals
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


static bool
jit_compile_op(JitCtx* jit, uint8_t opcode) {
    size_t code_offset = jit->code.offset;
    size_t buf_len = jit->buf->len;
    bool supported = false;

    switch (opcode) {
#define OP_DEF(op, val, name, ...)   \
    case op:                         \
        supported = jit_##name(jit); \
        break;
#include "common/opcodes.def"
#undef OP_DEF
    }

    if (!supported) {
        reader_goto(&jit->code, code_offset);
        jit->buf->len = buf_len;
    }
    return supported;
}

static bool
is_control(uint8_t opcode) {
    switch (opcode) {
    case OP_BLOCK:
    case OP_LOOP:
    case OP_IF:
    case OP_ELSE:
    case OP_END:
    case OP_BR:
    case OP_BR_IF:
    case OP_BR_TABLE:
    case OP_RETURN:
        return true;
    }
    return false;
}

bool jit_count_fallbacks;
static uint64_t fallback_counts[256];

void
jit_print_fallbacks(void) {
    if (!jit_count_fallbacks)
        return;

    printf("fallbacks:\n");
    for (int opcode = 0; opcode < 256; ++opcode) {
        if (fallback_counts[opcode])
            printf("  %-20s %lu\n", opcode_name(opcode), fallback_counts[opcode]);
    }
}

static uint64_t*
jit_fallback(CallCtx* ctx, uint64_t* sp, uint64_t opcode_offset) {
    jit_set_stack_top(ctx, sp);
    reader_goto(&ctx->code, opcode_offset);
    ctx->pc = opcode_offset;

    uint8_t opcode = read_u8(&ctx->code);
    if (jit_count_fallbacks)
        fallback_counts[opcode]++;

    interp_step(ctx, opcode);
    return jit_stack_top(ctx);
}

static void
jit_emit_fallback(JitCtx* jit, uint8_t opcode, size_t opcode_offset) {
    Module* module = jit->vm->module;
    uint32_t pops = 0;
    uint32_t pushes = 0;
    opcode_stack_effect(opcode, &pops, &pushes);

    if (opcode == OP_CALL_INDIRECT) {
        Reader code = jit->code;
        FuncType* type = &module->types[read_u32_leb(&code)];
        pops += type->nparam;
        pushes += type->nresult;
    }

    jit_emit_runtime_call(jit, jit_fallback, opcode_offset);
    skip_immediates(opcode, &jit->code);
    jit->height = jit->height - pops + pushes;
}

void
jit_emit_runtime_call(JitCtx* jit, JitRuntimeFn fn, uint64_t arg) {
    x86_mov_r64_r64(jit->buf, RDI, R13);
    x86_mov_r64_r64(jit->buf, RSI, RBX);
    x86_mov_r64_imm64(jit->buf, RDX, arg);
    x86_mov_r64_imm64(jit->buf, RAX, (uint64_t)fn);
    x86_call_r64(jit->buf, RAX);
    x86_mov_r64_r64(jit->buf, RBX, RAX);
}

void
jit_set_stack_top(CallCtx* ctx, uint64_t* sp) {
    ctx->stack->top = sp - ctx->stack->data;
}

uint64_t*
jit_stack_top(CallCtx* ctx) {
    return &ctx->stack->data[ctx->stack->top];
}

static void
jitctx_init(JitCtx* jit, CodeBuf* buf, VM* vm, uint64_t func_idx) {
    Func* func = &vm->module->funcs[func_idx];
    memset(jit, 0, sizeof(*jit));
    jit->buf = buf;
    jit->vm = vm;
    jit->func = func;

    reader_init(&jit->code, func->code, func->code_size);

    label_stack_init(&jit->labels, func->code_size + 1);
    jit->label_offsets = balloc(func->code_size + 1, sizeof(size_t));
    jit->fixups = balloc(func->code_size, sizeof(JitFixup));
    CHECK(jit->label_offsets && jit->fixups, "out of memory\n");

    Label entry = {
            .kind = LABEL_BLOCK,
            .arity = vm->module->types[func->type_idx].nresult,
            .end = func->code_size - 1,
    };
    label_push(&jit->labels, entry);
}
