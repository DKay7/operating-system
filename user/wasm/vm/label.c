#include <inc/lib.h>

#include "vm/label.h"
#include "common/check.h"
#include "common/wasm_types.h"

void
label_init(Label* label, LabelKind kind, Func* func, Reader* code, uint32_t height) {
    size_t opcode_offset = code->offset - 1;
    uint8_t blocktype = read_u8(code);

    label->kind = kind;
    label->arity = blocktype == BLOCK_TYPE_EMPTY ? 0 : 1;
    label->height = height;
    label->start = code->offset;
    label->end = func->block_end[opcode_offset];
    label->else_start = func->block_else[opcode_offset];
}

void
label_stack_init(LabelStack* labels, uint32_t size) {
    labels->data = balloc(size, sizeof(Label));
    CHECK(labels->data, "out of memory\n");
    labels->size = size;
    labels->top = 0;
}

void
label_push(LabelStack* labels, Label label) {
    CHECK(labels->top < labels->size, "label stack overflow\n");
    labels->data[labels->top++] = label;
}

Label
label_pop(LabelStack* labels) {
    CHECK(labels->top > 0, "label stack underflow\n");
    return labels->data[--labels->top];
}

Label*
label_get(LabelStack* labels, uint32_t depth) {
    CHECK(depth < labels->top, "wrong label depth %u\n", depth);
    return &labels->data[labels->top - 1 - depth];
}
