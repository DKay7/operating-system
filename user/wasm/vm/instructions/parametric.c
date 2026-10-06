
#include "vm/interp_call.h"
#include "vm/vm.h"

// parametric
void
vm_drop(CallCtx* ctx) {
    pop(ctx->stack);
}

void
vm_select(CallCtx* ctx) {
    uint64_t cond = pop(ctx->stack);
    uint64_t if_true = pop(ctx->stack);
    uint64_t if_false = pop(ctx->stack);

    if (cond)
        push(ctx->stack, if_true);
    else
        push(ctx->stack, if_false);
}
