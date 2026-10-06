#pragma once

#include <inc/types.h>

// I dont like it but I have no idea how to do that otherwise
// keeping that simplicity
#define JIT_REGION_BASE  0x1000000000ULL
#define JIT_STAGING_BASE 0x1400000000ULL
#define JIT_CODE_BASE    0x1800000000ULL
#define JIT_CODE_SIZE    (1ULL << 22)   

typedef struct {
    uint8_t* staging; // RW
    uint8_t* code;    // RX
    size_t size;
    size_t len;
} CodeBuf;

void codebuf_init(CodeBuf* buf);
void codebuf_finalize(CodeBuf* buf);

void emit_u8(CodeBuf* buf, uint8_t value);
void emit_u32(CodeBuf* buf, uint32_t value);
void emit_u64(CodeBuf* buf, uint64_t value);
