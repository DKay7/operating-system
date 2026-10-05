#include <inc/string.h>

#include "parser/reader.h"
#include "inc/assert.h"

static void
reader_fail(Reader *reader) {
    reader->error = true;
    reader->offset = reader->size;
}

void
reader_init(Reader *reader, const uint8_t *buf, size_t size) {
    reader->buf = buf;
    reader->size = size;
    reader->offset = 0;
    reader->error = false;
}

uint8_t
read_u8(Reader *reader) {
    if (reader->offset >= reader->size) {
        reader_fail(reader);
        return 0;
    }
    return reader->buf[reader->offset++];
}

uint32_t
read_u32_le(Reader *reader) {
    const uint8_t *bytes = read_bytes(reader, 4);
    if (!bytes) return 0;
    return bytes[0] | bytes[1] << 8 | bytes[2] << 16 | (uint32_t)bytes[3] << 24;
}

/* A `bits`-wide number takes at most ceil(bits / 7) bytes */
static uint64_t
read_leb(Reader *reader, unsigned bits, bool is_signed) {
    uint64_t result = 0;

    for (unsigned shift = 0; shift < bits; shift += 7) {
        uint8_t byte = read_u8(reader);
        result |= (uint64_t)(byte & 0x7F) << shift;

        if (!(byte & 0x80)) {
            /* Sign-extend from the last byte's top bit */
            if (is_signed && shift + 7 < 64 && (byte & 0x40))
                result |= ~0ULL << (shift + 7);
            return result;
        }
    }

    /* Too many bytes */
    reader_fail(reader);
    return 0;
}

uint32_t
read_u32_leb(Reader *reader) {
    return read_leb(reader, 32, false);
}

int32_t
read_i32_leb(Reader *reader) {
    return read_leb(reader, 32, true);
}

int64_t
read_i64_leb(Reader *reader) {
    return read_leb(reader, 64, true);
}

const uint8_t *
read_bytes(Reader *reader, size_t count) {
    if (reader->error || count > reader_remaining(reader)) {
        reader_fail(reader);
        return NULL;
    }
    const uint8_t *bytes = reader->buf + reader->offset;
    reader->offset += count;
    return bytes;
}

bool
reader_expect(Reader *reader, const void *expected, size_t count) {
    const uint8_t *bytes = read_bytes(reader, count);
    return bytes && !memcmp(bytes, expected, count);
}

const uint8_t *
read_name(Reader *reader, uint32_t *length) {
    *length = read_u32_leb(reader);
    const uint8_t *name = read_bytes(reader, *length);
    if (!name) *length = 0;
    return name;
}

uint32_t
read_vec_len(Reader *reader) {
    uint32_t count = read_u32_leb(reader);
    if (count > reader_remaining(reader)) {
        reader_fail(reader);
        return 0;
    }
    return count;
}

void
reader_sub(Reader *reader, size_t size, Reader *sub) {
    const uint8_t *bytes = read_bytes(reader, size);
    reader_init(sub, bytes, bytes ? size : 0);
    sub->error = reader->error;
}

void
reader_goto(Reader *reader, size_t offset) {
    if (offset > reader->size) {
        reader_fail(reader);
        return;
    }
    reader->offset = offset;
}

bool
reader_skip(Reader *reader, size_t offset) {
    if (offset > reader->size - reader->offset)
        return false;

    reader->offset += offset;
    return true;
}
