#include "common/check.h"
#include "parser/reader.h"
#include "vm/interp_call.h"
#include "vm/stack.h"
#include "vm/vm.h"

#define TYPE_u(width) uint##width##_t
#define TYPE_s(width) int##width##_t

// constants
void
vm_i32_const(CallCtx* ctx) {
    push(ctx->stack, (uint32_t)read_i32_leb(&ctx->code));
}

void
vm_i64_const(CallCtx* ctx) {
    push(ctx->stack, (uint64_t)read_i64_leb(&ctx->code));
}

// god forgive me for the code below
// laziness is a power for DDD (define-driven-development)

#define EQZ(width)                               \
    void vm_i##width##_eqz(CallCtx* ctx) {       \
        uint##width##_t value = pop(ctx->stack); \
        push(ctx->stack, value == 0);            \
    }

#define CMP(width, name, op, sign)                \
    void vm_i##width##_##name(CallCtx* ctx) {     \
        TYPE_##sign(width) rhs = pop(ctx->stack); \
        TYPE_##sign(width) lhs = pop(ctx->stack); \
        push(ctx->stack, lhs op rhs);             \
    }

// clang-format off
EQZ(32);                 EQZ(64);
CMP(32, eq, ==, u);      CMP(64, eq, ==, u);
CMP(32, ne, !=, u);      CMP(64, ne, !=, u);
CMP(32, lt_s, <, s);     CMP(64, lt_s, <, s);
CMP(32, lt_u, <, u);     CMP(64, lt_u, <, u);
CMP(32, gt_s, >, s);     CMP(64, gt_s, >, s);
CMP(32, gt_u, >, u);     CMP(64, gt_u, >, u);
CMP(32, le_s, <=, s);    CMP(64, le_s, <=, s);
CMP(32, le_u, <=, u);    CMP(64, le_u, <=, u);
CMP(32, ge_s, >=, s);    CMP(64, ge_s, >=, s);
CMP(32, ge_u, >=, u);    CMP(64, ge_u, >=, u);
// clang-format on

// wasm wants the width iv value = 0
// clang-format off
static uint32_t clz32(uint32_t value) { return value ? __builtin_clz(value) : 32; }
static uint64_t clz64(uint64_t value) { return value ? __builtin_clzll(value) : 64; }
static uint32_t ctz32(uint32_t value) { return value ? __builtin_ctz(value) : 32; }
static uint64_t ctz64(uint64_t value) { return value ? __builtin_ctzll(value) : 64; }
static uint32_t popcnt32(uint32_t value) { return __builtin_popcount(value); }
static uint64_t popcnt64(uint64_t value) { return __builtin_popcountll(value); }
// clang-format on

#define BITCOUNT(width, name)                    \
    void vm_i##width##_##name(CallCtx* ctx) {    \
        uint##width##_t value = pop(ctx->stack); \
        push(ctx->stack, name##width(value));    \
    }

// clang-format off
BITCOUNT(32, clz);       BITCOUNT(64, clz);
BITCOUNT(32, ctz);       BITCOUNT(64, ctz);
BITCOUNT(32, popcnt);    BITCOUNT(64, popcnt);
// clang-format on

#define BINOP(width, name, op)                 \
    void vm_i##width##_##name(CallCtx* ctx) {  \
        uint##width##_t rhs = pop(ctx->stack); \
        uint##width##_t lhs = pop(ctx->stack); \
        push(ctx->stack, lhs op rhs);          \
    }

// clang-format off
BINOP(32, add, +);       BINOP(64, add, +);
BINOP(32, sub, -);       BINOP(64, sub, -);
BINOP(32, mul, *);       BINOP(64, mul, *);
BINOP(32, and, &);       BINOP(64, and, &);
BINOP(32, or, |);        BINOP(64, or, |);
BINOP(32, xor, ^);       BINOP(64, xor, ^);
// clang-format on

#define SHIFT(width, name, op, sign)                                    \
    void vm_i##width##_##name(CallCtx* ctx) {                           \
        uint##width##_t rhs = pop(ctx->stack);                          \
        TYPE_##sign(width) lhs = pop(ctx->stack);                       \
        push(ctx->stack, (uint##width##_t)(lhs op(rhs & (width - 1)))); \
    }

// clang-format off
SHIFT(32, shl, <<, u);   SHIFT(64, shl, <<, u);
SHIFT(32, shr_s, >>, s); SHIFT(64, shr_s, >>, s);
SHIFT(32, shr_u, >>, u); SHIFT(64, shr_u, >>, u);
// clang-format on

//
// & (width - 1) is needed since wasm says 1 << 33 is the same is 1 << 1

#define ROT(width, name, op, back_op)                                                               \
    void vm_i##width##_##name(CallCtx* ctx) {                                                       \
        uint##width##_t rhs = pop(ctx->stack);                                                      \
        uint##width##_t lhs = pop(ctx->stack);                                                      \
        uint##width##_t k = rhs & (width - 1);                                                      \
        push(ctx->stack, (uint##width##_t)((lhs op k) | (lhs back_op((width - k) & (width - 1))))); \
    }

// clang-format off
ROT(32, rotl, <<, >>);   ROT(64, rotl, <<, >>);
ROT(32, rotr, >>, <<);   ROT(64, rotr, >>, <<);
// clang-format on


#define DIV_U(width, name, op)                       \
    void vm_i##width##_##name(CallCtx* ctx) {        \
        uint##width##_t rhs = pop(ctx->stack);       \
        uint##width##_t lhs = pop(ctx->stack);       \
        CHECK(rhs != 0, "integer divide by zero\n"); \
        push(ctx->stack, lhs op rhs);                \
    }

#define DIV_S(width)                                                          \
    void vm_i##width##_div_s(CallCtx* ctx) {                                  \
        int##width##_t rhs = pop(ctx->stack);                                 \
        int##width##_t lhs = pop(ctx->stack);                                 \
        CHECK(rhs != 0, "integer divide by zero\n");                          \
        CHECK(!(lhs == INT##width##_MIN && rhs == -1), "integer overflow\n"); \
        push(ctx->stack, (uint##width##_t)(lhs / rhs));                       \
    }

#define REM_S(width)                                         \
    void vm_i##width##_rem_s(CallCtx* ctx) {                 \
        int##width##_t rhs = pop(ctx->stack);                \
        int##width##_t lhs = pop(ctx->stack);                \
        CHECK(rhs != 0, "integer divide by zero\n");         \
        int##width##_t result = (rhs == -1) ? 0 : lhs % rhs; \
        push(ctx->stack, (uint##width##_t)result);           \
    }

// clang-format off
DIV_U(32, div_u, /);     DIV_U(64, div_u, /);
DIV_U(32, rem_u, %);     DIV_U(64, rem_u, %);
DIV_S(32);               DIV_S(64);
REM_S(32);               REM_S(64);
// clang-format on

