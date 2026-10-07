
#include "vm/jit/jit.h"

// a little hack for default jit-functions

#define OP_DEF(op, val, name, ...)                        \
    __attribute__((weak)) bool jit_##name(JitCtx* jit) { \
        (void)jit;                                        \
        return false;                                     \
    }
#include "common/opcodes.def"
#undef OP_DEF
