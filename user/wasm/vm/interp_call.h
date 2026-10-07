#pragma once

#include "common/check.h"
#include "vm/vm.h"

void interp_step(CallCtx* ctx, uint8_t opcode);

void callctx_init(CallCtx* call_ctx, VM* vm, uint32_t func_idx);
void callctx_leave(CallCtx* call_ctx);
uint32_t call_get_locals_base(CallCtx* call_ctx);
void callctx_dump(CallCtx* call_ctx);

#define NOT_IMPLEMENTED()                                  \
    do {                                                   \
        callctx_dump(ctx);                                 \
        CHECK(false, "%s is not implemented\n", __func__); \
    } while (0)

#define OP_DEF(op, val, name, ...) void vm_##name(CallCtx* ctx);
#include "common/opcodes.def"
#undef OP_DEF
