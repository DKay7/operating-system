#include "vm/interp_call.h"
#include "vm/vm.h"


void
vm_i32_wrap_i64(CallCtx* ctx) {
    uint64_t value = pop(ctx->stack);
    push(ctx->stack, (uint32_t)value);
}

void
vm_i64_extend_i32_s(CallCtx* ctx) {
    uint32_t value = pop(ctx->stack);
    int64_t extended = (int64_t)value;

    push(ctx->stack, extended);
}

void
vm_i64_extend_i32_u(CallCtx* ctx) {
    (void)ctx;
    // stack contains uint64_t values
    // wich means that uint32_t values are already zext'ed
}
