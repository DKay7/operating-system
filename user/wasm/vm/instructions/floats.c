#include "vm/interp_call.h"
#include "vm/vm.h"

// doom mostly do not use floats,
// so i dont care until i run it and check if it works
// and i will care only if it doesnt
#define OP_DEF(op, val, name, ...)                              \
    void vm_##name(CallCtx* ctx) {                              \
        callctx_dump(ctx);                                      \
        CHECK(false, "float instructions are not supported\n"); \
    }
#include "common/float_opcodes.def"
#undef OP_DEF
