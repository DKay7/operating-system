#pragma once

#include <inc/types.h>

#include "vm/vm.h"

typedef enum {
    WASI_ESUCCESS = 0,
    WASI_EBADF = 8,
    WASI_EINVAL = 28,
    WASI_ENOENT = 44,
    WASI_ENOSYS = 52,
} WasiErrno;

typedef uint64_t (*HostFn)(VM* vm, uint64_t* args);

typedef struct {
    const char* module;
    const char* name;
    HostFn fn;
} HostFunc;

HostFn wasi_lookup(const uint8_t* module, uint32_t module_len, const uint8_t* name, uint32_t name_len);
void wasi_set_args(int argc, char** argv);
