#include <inc/lib.h>

#include "common/check.h"
#include "vm/jit/codebuf.h"

void
codebuf_init(CodeBuf* buf) {
    uint8_t* base = (uint8_t*)JIT_REGION_BASE;
    uint8_t* staging = (uint8_t*)JIT_STAGING_BASE;
    uint64_t size = JIT_CODE_SIZE;

    int res = sys_alloc_region(0, base, size, PROT_X | PROT_RW);
    CHECK(res == 0, "jit: can't allocate  buffer: %i\n", res);

    res = sys_map_region(0, base, 0, staging, size, PROT_RW);
    CHECK(res == 0, "jit: can't map staging: %i\n", res);

    // do not map code till we are finish with writing
    //
    buf->staging = staging;
    buf->code = NULL;
    buf->size = size;
    buf->len = 0;
}

void
codebuf_finalize(CodeBuf* buf) {
    uint8_t* code = (uint8_t*)JIT_CODE_BASE;
    uint8_t* base = (uint8_t*)JIT_REGION_BASE;

    // first, unmap staging
    // only then map code
    int res = sys_unmap_region(0, buf->staging, buf->size);
    CHECK(res == 0, "jit: can't unmap code buffer: %i\n", res);
    buf->staging = NULL;

    res = sys_map_region(0, base, 0, code, buf->size, PROT_R | PROT_X);
    CHECK(res == 0, "jit: can't map code buffer: %i\n", res);

    buf->code = code;
}

void
emit_u8(CodeBuf* buf, uint8_t value) {
    CHECK(buf->len < buf->size, "jit code buffer is full\n");
    buf->staging[buf->len++] = value;
}

void
emit_u32(CodeBuf* buf, uint32_t value) {
    // little endian
    for (int idx = 0; idx < sizeof(value); ++idx)
        emit_u8(buf, value >> (8 * idx));
}

void
emit_u64(CodeBuf* buf, uint64_t value) {
    for (int idx = 0; idx < sizeof(value); ++idx)
        emit_u8(buf, value >> (8 * idx));
}
