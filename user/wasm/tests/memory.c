#include <stdint.h>

static uint8_t buf[64];

static void
fill(uint32_t seed) {
    for (int i = 0; i < 64; ++i) {
        seed = seed * 1103515245 + 12345;
        buf[i] = seed >> 16;
    }
}

int64_t
mem_mix(int seed) {
    fill(seed);
    int64_t sum = 0;
    for (int i = 0; i < 48; ++i) {
        volatile int8_t* s8 = (int8_t*)(buf + i);
        volatile uint8_t* u8 = buf + i;
        volatile int16_t* s16 = (int16_t*)(buf + i);
        volatile uint16_t* u16 = (uint16_t*)(buf + i);
        volatile int32_t* s32 = (int32_t*)(buf + i);
        volatile uint32_t* u32 = (uint32_t*)(buf + i);
        volatile int64_t* s64 = (int64_t*)(buf + i);

        sum = sum * 31 + *s8;
        sum = sum * 31 + *u8;
        sum = sum * 31 + *s16;
        sum = sum * 31 + *u16;
        sum = sum * 31 + (int64_t)*s32;
        sum = sum * 31 + (int64_t)*u32;
        sum = sum * 31 + *s64;
        sum ^= (int32_t)*s8 + (uint32_t)*u16;

        *(volatile int8_t*)(buf + i + 1) = (int8_t)sum;
        *(volatile int16_t*)(buf + i + 2) = (int16_t)(sum >> 8);
        *(volatile int32_t*)(buf + i + 3) = (int32_t)(sum >> 16);
        *(volatile int64_t*)(buf + i + 5) = sum ^ 0x5555;
    }
    return sum;
}

int
mem_read(int addr) {
    return *(volatile int*)addr;
}
