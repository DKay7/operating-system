#include <inc/lib.h>

#include "common/opcodes.h"
#include "common/check.h"

// One function per immediate kind used in opcodes.def (spec 5.4)

static void
skip_none(Reader* code) {
    (void)code;
}

// wasm 1.0: 0x40 (no result) or a value type, one byte
static void
skip_blocktype(Reader* code) {
    read_u8(code);
}

// local, global, function or label index
static void
skip_idx(Reader* code) {
    read_u32_leb(code);
}

// vec(labelidx) + default labelidx
static void
skip_br_table(Reader* code) {
    uint32_t count = read_vec_len(code);
    for (uint32_t idx = 0; idx <= count; ++idx)
        read_u32_leb(code);
}

// typeidx + 0x00 (table index)
static void
skip_call_indirect(Reader* code) {
    read_u32_leb(code);
    read_u8(code);
}

// align + offset
static void
skip_memarg(Reader* code) {
    read_u32_leb(code);
    read_u32_leb(code);
}

// 0x00 (memory index) of memory.size / memory.grow
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

void
skip_immediates(uint8_t opcode, Reader* code) {
    switch (opcode) {
#define OP_DEF(op, val, name, imm) \
    case op:                       \
        skip_##imm(code);          \
        return;
#include "common/opcodes.def"
#undef OP_DEF
    }
    CHECK(false, "unknown opcode 0x%x\n", opcode);
}
