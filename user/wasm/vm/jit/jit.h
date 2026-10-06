#pragma once

#include <inc/types.h>

#include "parser/reader.h"
#include "vm/jit/codebuf.h"
#include "vm/vm.h"

typedef uint64_t* (*JitCode)(CallCtx* ctx, uint64_t* sp, uint64_t* locals);

typedef struct {
    CodeBuf* buf;
    VM* vm;
    Func* func;
    Reader code;
} JitCtx;

void jit_compile_module(VM* vm);

void jit_emit_prologue(JitCtx* jit);
void jit_emit_epilogue(JitCtx* jit);


#define OP_DEF(op, val, name, ...) bool jit_##name(JitCtx* jit);
#include "common/opcodes.def"
#undef OP_DEF
