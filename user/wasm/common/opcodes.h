#pragma once

#include <inc/types.h>

#include "parser/reader.h"

#define OP_DEF(op, val, ...) op = val,
typedef enum {
    #include "common/opcodes.def"
} Opcode;

enum {
    MISC_MEMORY_COPY = 0x0A,
    MISC_MEMORY_FILL = 0x0B,
};

#undef OP_DEF

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

void skip_immediates(uint8_t opcode, Reader* code);

void opcode_stack_effect(uint8_t opcode, uint32_t* pops, uint32_t* pushes);
