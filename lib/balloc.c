#include "inc/lib.h"
#include <inc/memlayout.h>

// simple bump allocator

static void* cur = 0;

void*
balloc(size_t n, size_t unit_size) {

    // on first call we lazily allocate heap
    if (cur == 0) {
        int ret = sys_alloc_region(0, (void*)HEAP_START, HEAP_SIZE, PROT_RW);
        if (ret != 0)
            return NULL;

        cur = (void*)HEAP_START;
    }

    size_t size;
    if (__builtin_mul_overflow(n, unit_size, &size) || size > HEAP_SIZE)
        return NULL;
    size = ROUNDUP(size, 16);

    if (size > HEAP_END - (uintptr_t)cur)
        return NULL; // out of memory lol

    void* addr = cur;
    cur += size;

    return addr;
}

void
free(void* ptr) {
    (void)ptr;
    // lol
}

