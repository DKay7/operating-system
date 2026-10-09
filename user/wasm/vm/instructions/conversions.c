#include "vm/interp_call.h"
#include "vm/vm.h"


void
vm_i32_wrap_i64(CallCtx* ctx) {
    uint64_t value = pop(ctx->stack);
    push(ctx->stack, (uint32_t)value);
}

void
vm_i64_extend_i32_s(CallCtx* ctx) {
    int32_t value = pop(ctx->stack);
    int64_t extended = (int64_t)value;

    push(ctx->stack, extended);
}

void
vm_i64_extend_i32_u(CallCtx* ctx) {
    (void)ctx;
    // stack contains uint64_t values
    // wich means that uint32_t values are already zext'ed
}

void
vm_i32_extend8_s(CallCtx* ctx) {
    int8_t value = pop(ctx->stack);
    int32_t extended = (int32_t)value;

    push(ctx->stack, (uint32_t)extended);
}

void
vm_i32_extend16_s(CallCtx* ctx) {
    int16_t value = pop(ctx->stack);
    int32_t extended = (int32_t)value;

    push(ctx->stack, (uint32_t)extended);
}

void
vm_i64_extend8_s(CallCtx* ctx) {
    int8_t value = pop(ctx->stack);
    int64_t extended = (int64_t)value;

    push(ctx->stack, (uint64_t)extended);
}

void
vm_i64_extend16_s(CallCtx* ctx) {
    int16_t value = pop(ctx->stack);
    int64_t extended = (int64_t)value;

    push(ctx->stack, (uint64_t)extended);
}

void
vm_i64_extend32_s(CallCtx* ctx) {
    int32_t value = pop(ctx->stack);
    int64_t extended = (int64_t)value;

    push(ctx->stack, (uint64_t)extended);
}
