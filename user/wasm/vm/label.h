#pragma once

#include <inc/types.h>

#include "parser/reader.h"
#include "common/wasm_types.h"

/*
 * Label of an active block/loop/if, the target of `br`.
 *
 * Every call starts with an entry label for the function body itself
 * (arity = number of results, end = end of the code), as in the spec:
 * the last `end` of the body pops it, and `return` is a br to it.
 *
 * Branching to a label (br N, N = 0 is the innermost):
 *   - keep `arity` values from the top of the operand stack,
 *     drop everything above `height`, push the kept values back;
 *   - block/if: continue at `end` and drop the label with all inner ones;
 *   - loop: continue at `start`, the loop label stays (arity of a loop is 0).
 */
// Which instruction opened the label; decides where `br` jumps
typedef enum {
    LABEL_BLOCK, 
    LABEL_LOOP, 
    LABEL_IF,  
} LabelKind;

typedef struct {
    LabelKind kind;
    uint8_t arity;   //  0 or 1
    uint32_t height; 
    size_t start;   
    size_t end;
    size_t else_start; // if: right after `else`, 0 if there is no else
} Label;

// Labels of all active calls, like the value stack
typedef struct {
    Label* data;
    uint32_t size;
    uint32_t top; // first free slot
} LabelStack;

// Fill the label of a block of `func`; `code` is right after the block/loop/if
// opcode and is moved past the blocktype. `height` is the operand stack top on entry.
// end/else_start are taken from func->block_end/block_else (see parse_blocks).
void label_init(Label* label, LabelKind kind, Func* func, Reader* code, uint32_t height);

void label_stack_init(LabelStack* labels, uint32_t size);

void label_push(LabelStack* labels, Label label);
Label label_pop(LabelStack* labels);
Label* label_get(LabelStack* labels, uint32_t depth);
