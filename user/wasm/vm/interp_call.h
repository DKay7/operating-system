#pragma once

#include "common/check.h"
#include "vm/vm.h"

void callctx_init(CallCtx* call_ctx, VM* vm, uint32_t func_idx);
void callctx_leave(CallCtx* call_ctx);
uint32_t call_get_locals_base(CallCtx* call_ctx);
// Print function, position, locals and operands of the call
void callctx_dump(CallCtx* call_ctx);

// Stub for handlers that are not written yet; needs `ctx` in scope
#define NOT_IMPLEMENTED()                                  \
    do {                                                   \
        callctx_dump(ctx);                                 \
        CHECK(false, "%s is not implemented\n", __func__); \
    } while (0)

// Opcode handlers (interp_ops.c): void vm_<name>(CallCtx* ctx) for every OP_DEF
#define OP_DEF(op, val, name, ...) void vm_##name(CallCtx* ctx);
#include "common/opcodes.def"
#undef OP_DEF
