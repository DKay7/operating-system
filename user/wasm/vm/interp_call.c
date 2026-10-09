#include <inc/lib.h>
#include <stdint.h>

#include "vm/interp_call.h"
#include "common/check.h"
#include "common/opcodes.h"
#include "common/wasm_types.h"
#include "vm/jit/jit.h"
#include "vm/stack.h"
#include "vm/vm.h"
#include "vm/wasi.h"


static void call_host(VM* vm, Func* func);

void
interp_call(VM* vm, uint32_t func_idx) {
    Func* func = &vm->module->funcs[func_idx];

    if (func->imported) {
        call_host(vm, func);
        return;
    }

    CallCtx call;
    callctx_init(&call, vm, func_idx);

    CallCtx* ctx = &call;


    // use jit if available
    if (vm->jit_code && vm->jit_code[func_idx]) {
        JitCode code = vm->jit_code[func_idx];
        uint64_t* sp = code(ctx, &ctx->stack->data[ctx->stack->top], ctx->locals);
        ctx->stack->top = sp - ctx->stack->data;
        callctx_leave(ctx);
        return;
    }

    while (reader_remaining(&ctx->code) > 0) {
        ctx->pc = ctx->code.offset;
        interp_step(ctx, read_u8(&ctx->code));
    }

    callctx_leave(ctx);
}

static void
call_host(VM* vm, Func* func) {
    CHECK(func->imported == true, "what are you doin bro...");

    FuncType type = vm->module->types[func->type_idx];
    uint64_t res = ((HostFn)func->host)(vm, &vm->stack.data[vm->stack.top - type.nparam]);

    vm->stack.top -= type.nparam;

    if (type.nresult != 0)
        push(&vm->stack, res);
}

void
interp_step(CallCtx* ctx, uint8_t opcode) {
    switch (opcode) {
#define OP_DEF(op, val, name, ...) \
    case op:                       \
        vm_##name(ctx);            \
        return;
#include "common/opcodes.def"
#undef OP_DEF
    }

    callctx_dump(ctx);
    CHECK(false, "unsupported opcode 0x%x\n", opcode);
}


void
callctx_init(CallCtx* call_ctx, VM* vm, uint32_t func_idx) {
    Module* module = vm->module;
    Func* func = &module->funcs[func_idx];

    CHECK(++vm->call_depth <= MAX_CALL_DEPTH, "call stack exhausted\n");

    // params are already on stack
    FuncType* type = &module->types[func->type_idx];
    uint32_t locals_base = vm->stack.top - type->nparam;

    // alloc space for loclas
    for (uint32_t idx = 0; idx < func->nlocals; ++idx)
        push(&vm->stack, 0);

    call_ctx->vm = vm;
    call_ctx->stack = &vm->stack;
    call_ctx->func = func;
    call_ctx->locals = &vm->stack.data[locals_base];
    call_ctx->labels = &vm->labels;
    call_ctx->labels_base = vm->labels.top;
    reader_init(&call_ctx->code, func->code, func->code_size);

    // creating a implicit 'entry' block
    // to make my life a little bit easier
    Label entry = {
            .kind = LABEL_BLOCK,
            .arity = type->nresult,
            .height = vm->stack.top, // operands start right after the locals
            .start = 0,
            .end = func->code_size,
            .else_start = 0,
    };
    label_push(call_ctx->labels, entry);
}


void
callctx_leave(CallCtx* call_ctx) {
    FuncType* type = &call_ctx->vm->module->types[call_ctx->func->type_idx];

    // leave only the result in place of params and locals
    if (type->nresult > 0) {
        uint64_t result = pop(call_ctx->stack);
        call_ctx->stack->top = call_get_locals_base(call_ctx);
        push(call_ctx->stack, result);
    } else
        call_ctx->stack->top = call_get_locals_base(call_ctx);

    call_ctx->labels->top = call_ctx->labels_base;

    call_ctx->vm->call_depth--;
}

uint32_t
call_get_locals_base(CallCtx* call_ctx) {
    return call_ctx->locals - call_ctx->stack->data;
}

void
callctx_dump(CallCtx* call_ctx) {
    uint32_t func_idx = call_ctx->func - call_ctx->vm->module->funcs;
    uint8_t opcode = call_ctx->func->code[call_ctx->pc];
    printf("func %u, offset 0x%lx: %s\n", func_idx, call_ctx->pc, opcode_name(opcode));
}
