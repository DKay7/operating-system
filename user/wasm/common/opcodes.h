#pragma once

#include <inc/types.h>

#include "parser/reader.h"

#define OP_DEF(op, val, ...) op = val,
typedef enum {
    #include "common/opcodes.def"
} Opcode;

#undef OP_DEF

// "i32_add" for OP_I32_ADD, "?" for unknown bytes
static inline const char*
opcode_name(uint8_t opcode) {
    switch (opcode) {
#define OP_DEF(op, val, name, ...) \
    case op:                       \
        return #name;
#include "common/opcodes.def"
#undef OP_DEF
    }
    return "?";
}

// Skip the immediates of `opcode`; the reader must be right after the opcode byte
void skip_immediates(uint8_t opcode, Reader* code);
