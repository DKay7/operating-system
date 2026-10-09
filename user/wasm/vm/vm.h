#pragma once

#include "common/wasm_types.h"
#include "parser/reader.h"
#include "vm/stack.h"
#include "vm/label.h"

#define STACK_SIZE     (64 * 1024)
#define LABELS_SIZE    4096
#define MAX_CALL_DEPTH 300
#define WASM_PAGE_SIZE (64 * 1024)
#define MAX_MEM_PAGES  1024

typedef struct {
    Module* module;

    uint64_t* globals;

    uint8_t* memory;
    uint64_t mem_size;
    uint64_t mem_capacity;

    uint32_t* table;
    uint32_t table_size;

    Stack stack;
    LabelStack labels;

    uint32_t call_depth;

    void** jit_code;
} VM;

typedef struct {
    VM* vm;
    Stack* stack;
    Func* func;
    Reader code;
    size_t pc; // offset of the instruction being executed, for debug
    uint64_t* locals;

    LabelStack* labels;
    uint32_t labels_base; // labels of this call are [labels_base, labels->top)
} CallCtx;

void vm_init(VM* vm, Module* module);
bool find_export(Module* module, const char* name, uint32_t* func_idx);
void interp_call(VM* vm, uint32_t func_idx);
