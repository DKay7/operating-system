#ifndef JOS_USER_WASM_PARSER_READER_H
#define JOS_USER_WASM_PARSER_READER_H

#include <inc/types.h>

typedef struct {
    const uint8_t *buf;
    size_t size;
    size_t offset;
    bool error;
} Reader;

void reader_init(Reader *reader, const uint8_t *buf, size_t size);

static inline bool
reader_ok(const Reader *reader) {
    return !reader->error;
}

static inline size_t
reader_remaining(const Reader *reader) {
    return reader->size - reader->offset;
}

static inline bool
reader_eof(const Reader *reader) {
    return !reader->error && reader->offset == reader->size;
}

uint8_t read_u8(Reader *reader);
uint32_t read_u32_le(Reader *reader);

uint32_t read_u32_leb(Reader *reader);
int32_t read_i32_leb(Reader *reader);
int64_t read_i64_leb(Reader *reader);

const uint8_t *read_bytes(Reader *reader, size_t count);
bool reader_expect(Reader *reader, const void *expected, size_t count);
const uint8_t *read_name(Reader *reader, uint32_t *length);
uint32_t read_vec_len(Reader *reader);

void reader_sub(Reader *reader, size_t size, Reader *sub);

bool reader_skip(Reader *reader, size_t offset);
void reader_goto(Reader *reader, size_t offset);
#endif /* !JOS_USER_WASM_PARSER_READER_H */
