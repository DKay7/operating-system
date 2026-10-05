#include "parser/reader.h"
#include "vm/interp_call.h"
#include "vm/stack.h"
#include "vm/vm.h"

void
vm_local_get(CallCtx* ctx) {
    push(ctx->stack, ctx->locals[read_u32_leb(&ctx->code)]);
}
void
vm_local_set(CallCtx* ctx) {
    uint64_t value = pop(ctx->stack);
    ctx->locals[read_u32_leb(&ctx->code)] = value;
}

void
vm_local_tee(CallCtx* ctx) {
    uint64_t value = ctx->stack->data[ctx->stack->top - 1];
    ctx->locals[read_u32_leb(&ctx->code)] = value;
}

void
vm_global_get(CallCtx* ctx) {
    push(ctx->stack, ctx->vm->globals[read_u32_leb(&ctx->code)]);
}

void
vm_global_set(CallCtx* ctx) {
    uint64_t value = pop(ctx->stack);
    ctx->vm->globals[read_u32_leb(&ctx->code)] = value;
}
