#include <inc/lib.h>

#include "vm/stack.h"
#include "common/check.h"

void
stack_init(Stack* stack, uint32_t size) {
    stack->data = balloc(size, sizeof(uint64_t));
    CHECK(stack->data, "out of memory\n");
    stack->size = size;
    stack->top = 0;
}

void
push(Stack* stack, uint64_t value) {
    CHECK(stack->top < stack->size, "value stack overflow\n");
    stack->data[stack->top++] = value;
}

uint64_t
pop(Stack* stack) {
    CHECK(stack->top > 0, "value stack underflow\n");
    return stack->data[--stack->top];
}
