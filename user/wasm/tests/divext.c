#include <stdint.h>

__attribute__((noinline)) static int64_t
widen_s(int32_t x) {
    return x;
}

__attribute__((noinline)) static uint64_t
widen_u(uint32_t x) {
    return x;
}

int64_t
divext_mix(int n) {
    uint64_t acc = 0;
    for (int i = 1; i <= n; ++i) {
        uint32_t a = 0xFFFFFFF0u - i * 2654435761u;
        uint32_t b = (uint32_t)i * 7 + 3;
        int32_t s = (int32_t)(a / b) - i * 100000;
        acc = acc * 3 + (uint64_t)widen_s(s) + widen_u(a / b);
    }
    return (int64_t)acc;
}

unsigned
div_u(unsigned a, unsigned b) {
    return a / b;
}
