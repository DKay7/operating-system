int size(void) { return __builtin_wasm_memory_size(0); }
int grow(int n) { return __builtin_wasm_memory_grow(0, n); }
/* grow, write to the first new page, read it back plus the new size */
int grow_write(int n) {
    int old = __builtin_wasm_memory_grow(0, n);
    if (old < 0) return -1;
    volatile int *p = (volatile int *)(old * 65536);
    *p = 1000;
    return *p + __builtin_wasm_memory_size(0);
}
/* the new page must be zero */
int grow_read(int n) {
    int old = __builtin_wasm_memory_grow(0, n);
    volatile int *p = (volatile int *)(old * 65536 + 100);
    return *p;
}
