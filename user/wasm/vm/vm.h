#pragma once

#include "common/wasm_types.h"
#include "parser/reader.h"
#include "vm/stack.h"
#include "vm/label.h"

#define STACK_SIZE     (64 * 1024) // values on the value stack
#define LABELS_SIZE    4096        // labels of all active blocks
#define MAX_CALL_DEPTH 300         // interp_call recursion uses the JOS stack (64 KB)
#define WASM_PAGE_SIZE (64 * 1024)

// Runtime state of a module, shared by all calls
typedef struct {
    Module* module;

    uint64_t* globals;

    uint8_t* memory;
    uint64_t mem_size; // in bytes

    Stack stack;
    LabelStack labels;

    uint32_t call_depth;
} VM;

typedef struct {
    VM* vm;
    Stack* stack;     // == &vm->stack, for short push(ctx->stack, ...)
    Func* func;
    Reader code;      // current position in the function body
    size_t pc;        // offset of the instruction being executed, for debug
    uint64_t* locals; // params, then declared locals

    LabelStack* labels;   // == &vm->labels
    uint32_t labels_base; // labels of this call are [labels_base, labels->top),
                          // the first one is the entry label of the function body
} CallCtx;

void vm_init(VM* vm, Module* module);

// Returns false if there is no exported function with this name
bool find_export(Module* module, const char* name, uint32_t* func_idx);

// Arguments are taken from the value stack, the result (if any) is pushed back
void interp_call(VM* vm, uint32_t func_idx);

