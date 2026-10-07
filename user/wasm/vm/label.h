#pragma once

#include <inc/types.h>

#include "parser/reader.h"
#include "common/wasm_types.h"

typedef enum {
    LABEL_BLOCK,
    LABEL_LOOP,
    LABEL_IF,
} LabelKind;

typedef struct {
    LabelKind kind;
    uint8_t arity; //  0 or 1
    uint32_t height;
    size_t start;
    size_t end;
    size_t else_start;
} Label;

// Labels of all active calls
typedef struct {
    Label* data;
    uint32_t size;
    uint32_t top;
} LabelStack;

void label_init(Label* label, LabelKind kind, Func* func, Reader* code, uint32_t height);

void label_stack_init(LabelStack* labels, uint32_t size);

void label_push(LabelStack* labels, Label label);
Label label_pop(LabelStack* labels);
Label* label_get(LabelStack* labels, uint32_t depth);
