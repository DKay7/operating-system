#pragma once

#include <inc/types.h>

#include "parser/reader.h"
#include "vm/jit/codebuf.h"
#include "vm/jit/x86_64.h"
#include "vm/label.h"
#include "vm/vm.h"

typedef uint64_t* (*JitCode)(CallCtx* ctx, uint64_t* sp, uint64_t* locals);

typedef struct {
    size_t at;
    size_t target;
} JitFixup;

typedef struct {
    CodeBuf* buf;
    VM* vm;
    Func* func;
    Reader code;

    LabelStack labels;
    uint32_t height;

    size_t* label_offsets;
    JitFixup* fixups;
    uint32_t nfixups;
} JitCtx;


void jit_compile_module(VM* vm);

// push / pop a register on the wasm value stack (rbx), changing jit->height
void jit_wasm_push(JitCtx* jit, Reg reg);
void jit_wasm_pop(JitCtx* jit, Reg reg);
// the same, but jit->height stays: for code on a branch path (emit_branch)
void jit_wasm_push_raw(JitCtx* jit, Reg reg);
void jit_wasm_pop_raw(JitCtx* jit, Reg reg);

typedef uint64_t* (*JitRuntimeFn)(CallCtx* ctx, uint64_t* sp, uint64_t arg);
void jit_emit_runtime_call(JitCtx* jit, JitRuntimeFn fn, uint64_t arg);

void jit_set_stack_top(CallCtx* ctx, uint64_t* sp);
uint64_t* jit_stack_top(CallCtx* ctx);

void jit_emit_prologue(JitCtx* jit);
void jit_emit_epilogue(JitCtx* jit);


#define OP_DEF(op, val, name, ...) bool jit_##name(JitCtx* jit);
#include "common/opcodes.def"
#undef OP_DEF
