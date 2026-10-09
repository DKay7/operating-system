
#include <inc/string.h>

#include "common/check.h"
#include "common/opcodes.h"
#include "parser/reader.h"
#include "vm/interp_call.h"
#include "vm/stack.h"
#include "vm/vm.h"

// does the same as lea instruction in x86
// but im not gonna call it load_*
// since it does not loading
static uint8_t*
get_effective_addr(CallCtx* ctx, uint64_t size) {
    // ignore aligment
    read_u32_leb(&ctx->code);

    uint32_t offset = read_u32_leb(&ctx->code);

    // the address is an i32; the sum is done in 64, so it can't overflow
    uint64_t addr = (uint32_t)pop(ctx->stack);
    addr += offset;

    CHECK(addr + size <= ctx->vm->mem_size, "out of bounds memory access\n");

    return ctx->vm->memory + addr;
}

#define LOAD(name, width, memtype)                                \
    void name(CallCtx* ctx) {                                     \
        memtype value = 0;                                        \
        uint8_t* addr = get_effective_addr(ctx, sizeof(memtype)); \
        /* memcpy since addr may be unaligned*/                   \
        memcpy(&value, addr, sizeof(memtype));                    \
        push(ctx->stack, (uint##width##_t)value);                 \
    }

// clang-format off
LOAD(vm_i32_load,     32, uint32_t);   
LOAD(vm_i64_load,     64, uint64_t);

LOAD(vm_i32_load8_s,  32, int8_t);     
LOAD(vm_i64_load8_s,  64, int8_t);

LOAD(vm_i32_load8_u,  32, uint8_t);    
LOAD(vm_i64_load8_u,  64, uint8_t);

LOAD(vm_i32_load16_s, 32, int16_t);    
LOAD(vm_i64_load16_s, 64, int16_t);

LOAD(vm_i32_load16_u, 32, uint16_t);   
LOAD(vm_i64_load16_u, 64, uint16_t);

LOAD(vm_i64_load32_s, 64, int32_t);
LOAD(vm_i64_load32_u, 64, uint32_t);
// clang-format on
#undef LOAD


#define STORE(name, memtype)                                      \
    void name(CallCtx* ctx) {                                     \
        memtype value = (memtype)pop(ctx->stack);                 \
        uint8_t* addr = get_effective_addr(ctx, sizeof(memtype)); \
        /* memcpy since addr may be unaligned*/                   \
        memcpy(addr, &value, sizeof(memtype));                    \
    }

// clang-format off
STORE(vm_i32_store,   uint32_t);
STORE(vm_i64_store,   uint64_t);

STORE(vm_i32_store8,  uint8_t);
STORE(vm_i64_store8,  uint8_t);

STORE(vm_i32_store16, uint16_t);
STORE(vm_i64_store16, uint16_t);

STORE(vm_i64_store32, uint32_t);
// clang-format on
#undef STORE


void
vm_memory_size(CallCtx* ctx) {
    read_u8(&ctx->code); // always 0 in wasm1.0
    push(ctx->stack, (uint32_t)(ctx->vm->mem_size / WASM_PAGE_SIZE));
}

void
vm_memory_grow(CallCtx* ctx) {
    // not actually supported for now since it requires to have realloc
    // maybe layter...
    // for now just stor -1 indicating oom
    // which is totally fine
    // or maybe i care/...
    // fucking doom
    //
    read_u8(&ctx->code); // always 0 in wasm1.0
    uint64_t delta = pop(ctx->stack);
    uint64_t old_sz = ctx->vm->mem_size / WASM_PAGE_SIZE;
    if (old_sz + delta > ctx->vm->mem_capacity / WASM_PAGE_SIZE) {
        push(ctx->stack, (uint32_t)-1);
        return;
    }

    ctx->vm->mem_size += delta * WASM_PAGE_SIZE;
    push(ctx->stack, old_sz);
}

static void
vm_memory_copy(CallCtx* ctx) {

    // skip imm
    read_u32_leb(&ctx->code);
    read_u32_leb(&ctx->code);

    uint64_t n = pop(ctx->stack);
    uint64_t src = pop(ctx->stack);
    uint64_t dst = pop(ctx->stack);
    // skip checks since i dont care if dumb programmer makes mistakes
    // just write code with no bugs bro
    memmove(ctx->vm->memory + dst, ctx->vm->memory + src, n);
}

static void
vm_memory_fill(CallCtx* ctx) {
    // skip imm
    read_u32_leb(&ctx->code);

    uint64_t n = pop(ctx->stack);
    uint8_t val = pop(ctx->stack);
    uint64_t dst = pop(ctx->stack);
    // skip checks since i dont care if dumb programmer makes mistakes
    // just write code with no bugs bro
    memset(ctx->vm->memory + dst, val, n);
}

void
vm_misc(CallCtx* ctx) {
    uint32_t op = read_u32_leb(&ctx->code);
    switch (op) {
    case MISC_MEMORY_COPY:
        vm_memory_copy(ctx);
        return;
    case MISC_MEMORY_FILL:
        vm_memory_fill(ctx);
        return;
    }
    callctx_dump(ctx);
    CHECK(false, "unknown 0xFC opcode %u\n", op);
}
