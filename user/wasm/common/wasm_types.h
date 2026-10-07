#pragma once

#include <inc/types.h>

typedef enum : uint8_t {
    i32 = 0x7f,
    i64 = 0x7e,
    // f32 = 0x7d and f64 = 0x7c are not supported
} ValueType;

#define BLOCK_TYPE_EMPTY 0x40

typedef enum : uint8_t {
    type_const = 0x00,
    type_var = 0x01,
} MutType;


typedef enum : uint8_t {
    func = 0x00,
    table = 0x01,
    memory = 0x02,
    global = 0x03
} ExportType;

typedef struct {
    ValueType *param_types;
    ValueType *result_types;
    uint32_t nparam, nresult;
} FuncType;

typedef struct {
    uint32_t type_idx; // idx in module types
    uint32_t nlocals;
    ValueType *local_types;
    const uint8_t *code;
    uint32_t code_size;

    uint32_t *block_end;
    uint32_t *block_else;
} Func;

typedef struct {
    const uint8_t *name;
    uint32_t name_len;
    ExportType kind;
    uint32_t index;
} Export;

typedef struct {
    const uint8_t *instructions;
    uint32_t len;
} Expression;

typedef struct {
    ValueType valtype;
    MutType mut;
    Expression init;
} Global;

typedef struct {
    FuncType *types;
    uint32_t ntypes;

    uint32_t *func_type_idx;
    uint32_t nfunc_type_idx;

    Func *funcs;
    uint32_t nfuncs, nimported_funcs;
    Global *globals;
    uint32_t nglobals;
    Export *exports;
    uint32_t nexports;
    // Data *data;
    uint32_t ndata;
    bool has_memory;
    uint32_t mem_min, mem_max;
    bool has_start;
    uint32_t start_func;
} Module;
