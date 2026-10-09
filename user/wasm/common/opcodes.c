#include <inc/lib.h>

#include "common/opcodes.h"
#include "common/check.h"


static void
skip_none(Reader* code) {
    (void)code;
}

static void
skip_blocktype(Reader* code) {
    read_u8(code);
}

static void
skip_idx(Reader* code) {
    read_u32_leb(code);
}

static void
skip_br_table(Reader* code) {
    uint32_t count = read_vec_len(code);
    for (uint32_t idx = 0; idx <= count; ++idx)
        read_u32_leb(code);
}

static void
skip_call_indirect(Reader* code) {
    read_u32_leb(code);
    read_u8(code);
}

static void
skip_memarg(Reader* code) {
    read_u32_leb(code);
    read_u32_leb(code);
}

static void
skip_memidx(Reader* code) {
    read_u8(code);
}

static void
skip_i32(Reader* code) {
    read_i32_leb(code);
}

static void
skip_i64(Reader* code) {
    read_i64_leb(code);
}

static void
skip_f32(Reader* code) {
    read_bytes(code, 4);
}

static void
skip_f64(Reader* code) {
    read_bytes(code, 8);
}

static void
skip_misc(Reader* code) {
    uint32_t op = read_u32_leb(code);
    switch (op) {
    case MISC_MEMORY_COPY:
        read_u8(code);
        read_u8(code);
        return;
    case MISC_MEMORY_FILL:
        read_u8(code);
        return;
    }
    CHECK(false, "unknown 0xFC opcode %u\n", op);
}

void
skip_immediates(uint8_t opcode, Reader* code) {
    switch (opcode) {
#define OP_DEF(op, val, name, imm, ...) \
    case op:                       \
        skip_##imm(code);          \
        return;
#include "common/opcodes.def"
#undef OP_DEF
    }
    CHECK(false, "unknown opcode 0x%x\n", opcode);
}

void
opcode_stack_effect(uint8_t opcode, uint32_t* pops, uint32_t* pushes) {
    switch (opcode) {
#define OP_DEF(op, val, name, imm, npops, npushes) \
    case op:                                       \
        *pops = npops;                             \
        *pushes = npushes;                         \
        return;
#include "common/opcodes.def"
#undef OP_DEF
    }
    CHECK(false, "unknown opcode 0x%x\n", opcode);
}
