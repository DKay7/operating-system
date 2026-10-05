#pragma once

#include <inc/types.h>

/*
 * Value stack
 *   ... | params | locals | operands ... |
 */
typedef struct {
    uint64_t* data;
    uint32_t size;
    uint32_t top;
} Stack;

void stack_init(Stack* stack, uint32_t size);

void push(Stack* stack, uint64_t value);
uint64_t pop(Stack* stack);

