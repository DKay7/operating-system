int
shift_mix(int n) {
    unsigned acc = 0x9E3779B9u;
    int sacc = -123456789;
    for (int i = 0; i < n; ++i) {
        volatile int k = i * 7;
        acc ^= (acc << (k & 31)) + (acc >> ((k + 3) & 31));
        sacc ^= (sacc >> (k & 31)) - (sacc << (k & 31));
        acc += (unsigned)sacc;
    }
    return (int)acc;
}

int
shift_wide(int x, int k) {
    return (x << k) ^ ((unsigned)x >> k) ^ (x >> k);
}
