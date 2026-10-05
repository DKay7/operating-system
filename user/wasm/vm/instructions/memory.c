// Interpreter handlers: loads, stores, memory.size/grow

#include <inc/string.h>

#include "common/check.h"
#include "parser/reader.h"
#include "vm/interp_call.h"
#include "vm/stack.h"
#include "vm/vm.h"

// memory: all have memarg immediate (align:u32, offset:u32)

void
vm_i32_load(CallCtx* ctx) {
    // memarg immediate: alignment hint (ignored) and static offset
    read_u32_leb(&ctx->code);
    uint32_t offset = read_u32_leb(&ctx->code);

    // effective address = dynamic address from the stack + static offset, in 64 bits
    uint64_t addr = (uint64_t)(uint32_t)pop(ctx->stack) + offset;
    CHECK(addr + sizeof(uint32_t) <= ctx->vm->mem_size, "out of bounds memory access\n");

    uint32_t value;
    memcpy(&value, ctx->vm->memory + addr, sizeof(value)); // may be unaligned
    push(ctx->stack, value);
}

void
vm_i64_load(CallCtx* ctx) {
    NOT_IMPLEMENTED();
}

void
vm_i32_load8_s(CallCtx* ctx) {
    NOT_IMPLEMENTED();
}

void
vm_i32_load8_u(CallCtx* ctx) {
    NOT_IMPLEMENTED();
}

void
vm_i32_load16_s(CallCtx* ctx) {
    NOT_IMPLEMENTED();
}

void
vm_i32_load16_u(CallCtx* ctx) {
    NOT_IMPLEMENTED();
}

void
vm_i64_load8_s(CallCtx* ctx) {
    NOT_IMPLEMENTED();
}

void
vm_i64_load8_u(CallCtx* ctx) {
    NOT_IMPLEMENTED();
}

void
vm_i64_load16_s(CallCtx* ctx) {
    NOT_IMPLEMENTED();
}

void
vm_i64_load16_u(CallCtx* ctx) {
    NOT_IMPLEMENTED();
}

void
vm_i64_load32_s(CallCtx* ctx) {
    NOT_IMPLEMENTED();
}

void
vm_i64_load32_u(CallCtx* ctx) {
    NOT_IMPLEMENTED();
}

void
vm_i32_store(CallCtx* ctx) {
    NOT_IMPLEMENTED();
}

void
vm_i64_store(CallCtx* ctx) {
    NOT_IMPLEMENTED();
}

void
vm_i32_store8(CallCtx* ctx) {
    NOT_IMPLEMENTED();
}

void
vm_i32_store16(CallCtx* ctx) {
    NOT_IMPLEMENTED();
}

void
vm_i64_store8(CallCtx* ctx) {
    NOT_IMPLEMENTED();
}

void
vm_i64_store16(CallCtx* ctx) {
    NOT_IMPLEMENTED();
}

void
vm_i64_store32(CallCtx* ctx) {
    NOT_IMPLEMENTED();
}

void
vm_memory_size(CallCtx* ctx) {
    NOT_IMPLEMENTED();
}

void
vm_memory_grow(CallCtx* ctx) {
    NOT_IMPLEMENTED();
}
