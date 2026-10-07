#include <inc/lib.h>
#include <stdint.h>

#include "vm/interp_call.h"
#include "common/check.h"
#include "common/opcodes.h"
#include "vm/jit/jit.h"
#include "vm/stack.h"
#include "vm/vm.h"

void
interp_call(VM* vm, uint32_t func_idx) {
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

    // `return` may leave from inside blocks: drop their labels
    call_ctx->labels->top = call_ctx->labels_base;

    call_ctx->vm->call_depth--;
}

uint32_t
call_get_locals_base(CallCtx* call_ctx) {
    return call_ctx->locals - call_ctx->stack->data;
}

static const char*
valtype_name(uint8_t type) {
    return type == i32 ? "i32" : type == i64 ? "i64" :
                                               "?";
}

static void
print_value(uint8_t type, uint64_t value) {
    if (type == i32)
        printf("%d", (int32_t)value);
    else
        printf("%ld", (int64_t)value);
    printf(" (0x%lx)", value);
}

#define DUMP_MAX_VALUES 16

void
callctx_dump(CallCtx* call_ctx) {
    Module* module = call_ctx->vm->module;
    Func* function = call_ctx->func;
    FuncType* type = &module->types[function->type_idx];
    uint32_t func_idx = function - module->funcs;

    printf("=== call depth %u: func %u", call_ctx->vm->call_depth, func_idx);
    for (uint32_t idx = 0; idx < module->nexports; ++idx) {
        Export* export = &module->exports[idx];
        if (export->kind == func && export->index == func_idx)
            printf(" '%.*s'", (int)export->name_len, export->name);
    }
    printf(" (");
    for (uint32_t idx = 0; idx < type->nparam; ++idx)
        printf("%s%s", idx ? ", " : "", valtype_name(type->param_types[idx]));
    printf(") -> %s\n", type->nresult ? valtype_name(type->result_types[0]) : "()");

    // position: the instruction being executed
    uint8_t opcode = function->code[call_ctx->pc];
    printf("  at offset 0x%lx of %u: opcode 0x%02x %s\n",
           call_ctx->pc, function->code_size, opcode, opcode_name(opcode));

    // locals: params first, then declared locals
    uint32_t nlocals = type->nparam + function->nlocals;
    printf("  locals (%u):\n", nlocals);
    for (uint32_t idx = 0; idx < nlocals && idx < DUMP_MAX_VALUES; ++idx) {
        bool is_param = idx < type->nparam;
        uint8_t local_type = is_param ? type->param_types[idx] : function->local_types[idx - type->nparam];
        printf("    [%u] %s %s = ", idx, is_param ? "param" : "local", valtype_name(local_type));
        print_value(local_type, call_ctx->locals[idx]);
        printf("\n");
    }
    if (nlocals > DUMP_MAX_VALUES)
        printf("    ... %u more\n", nlocals - DUMP_MAX_VALUES);

    // operands of this call: from the end of locals to the stack top
    uint32_t operands_base = call_get_locals_base(call_ctx) + nlocals;
    uint32_t noperands = call_ctx->stack->top - operands_base;
    printf("  operands (%u, top last):\n", noperands);
    uint32_t first = noperands > DUMP_MAX_VALUES ? noperands - DUMP_MAX_VALUES : 0;
    if (first > 0)
        printf("    ... %u below\n", first);
    for (uint32_t idx = first; idx < noperands; ++idx) {
        printf("    [%u] ", idx);
        print_value(i64, call_ctx->stack->data[operands_base + idx]); // types are not tracked
        printf("\n");
    }
}
