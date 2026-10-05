#pragma once

#include "parser/reader.h"
#include "common/wasm_types.h"

typedef struct {
    Reader reader;
    Module module;
    uint8_t last_section_id; // to check that sections go in increasing order
} Parser;



Parser* parse_wasm(const uint8_t* buf, size_t size);
