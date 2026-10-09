#include <inc/lib.h>
#include <inc/x86.h>

#include "common/check.h"
#include "vm/jit/jit.h"
#include "vm/wasi.h"

#define PREOPEN_FD   3
#define FILE_FD_BASE 4

enum {
    FILETYPE_CHARACTER_DEVICE = 2,
    FILETYPE_DIRECTORY = 3,
    FILETYPE_REGULAR_FILE = 4,
};

enum {
    OFLAGS_CREAT = 1,
    OFLAGS_TRUNC = 8,
};

enum {
    WHENCE_SET = 0,
    WHENCE_CUR = 1,
    WHENCE_END = 2,
};

typedef struct {
    uint32_t buf;
    uint32_t len;
} WasiIovec;

static int wasi_argc;
static char** wasi_argv;

void
wasi_set_args(int argc, char** argv) {
    wasi_argc = argc;
    wasi_argv = argv;
}

static uint8_t*
mem(VM* vm, uint64_t addr, uint64_t size) {
    CHECK(addr + size <= vm->mem_size, "wasi: out of bounds memory access\n");
    return vm->memory + addr;
}

static void
store(VM* vm, uint64_t addr, const void* value, size_t size) {
    memcpy(mem(vm, addr, size), value, size);
}

static void
store_u32(VM* vm, uint64_t addr, uint32_t value) {
    store(vm, addr, &value, sizeof(value));
}

static void
store_u64(VM* vm, uint64_t addr, uint64_t value) {
    store(vm, addr, &value, sizeof(value));
}

static uint64_t
load_u64(VM* vm, uint64_t addr) {
    uint64_t value;
    memcpy(&value, mem(vm, addr, sizeof(value)), sizeof(value));
    return value;
}

static WasiIovec*
iovecs(VM* vm, uint32_t ptr, uint32_t len) {
    return (WasiIovec*)mem(vm, ptr, len * sizeof(WasiIovec));
}

static uint64_t
now_ns(void) {
    static uint64_t freq;
    if (!freq)
        freq = sys_get_tsc_freq();

    uint64_t tsc = read_tsc();
    const uint64_t mult = 1000000000ULL;
    return tsc / freq * mult + tsc % freq * mult / freq;
}

static uint64_t
wasi_args_get(VM* vm, uint64_t* args) {
    uint32_t argv_ptr = args[0];
    uint32_t buf_ptr = args[1];

    for (int idx = 0; idx < wasi_argc; ++idx) {
        size_t len = strlen(wasi_argv[idx]) + 1;
        store(vm, buf_ptr, wasi_argv[idx], len);
        store_u32(vm, argv_ptr + idx * sizeof(uint32_t), buf_ptr);
        buf_ptr += len;
    }
    return WASI_ESUCCESS;
}

static uint64_t
wasi_args_sizes_get(VM* vm, uint64_t* args) {
    uint32_t buf_size = 0;
    for (int idx = 0; idx < wasi_argc; ++idx)
        buf_size += strlen(wasi_argv[idx]) + 1;

    store_u32(vm, args[0], wasi_argc);
    store_u32(vm, args[1], buf_size);
    return WASI_ESUCCESS;
}

// no envs
static uint64_t
wasi_environ_get(VM* vm, uint64_t* args) {
    return WASI_ESUCCESS;
}

static uint64_t
wasi_environ_sizes_get(VM* vm, uint64_t* args) {
    store_u32(vm, args[0], 0); // size = 0
    store_u32(vm, args[1], 0);
    return WASI_ESUCCESS;
}

static uint64_t
wasi_clock_time_get(VM* vm, uint64_t* args) {
    store_u64(vm, args[2], now_ns());
    return WASI_ESUCCESS;
}

static uint64_t
wasi_fd_close(VM* vm, uint64_t* args) {
    uint32_t fd = args[0];
    if (fd < FILE_FD_BASE)
        return WASI_ESUCCESS;
    return close(fd - FILE_FD_BASE) < 0 ? WASI_EBADF : WASI_ESUCCESS;
}

static uint64_t
wasi_fd_fdstat_get(VM* vm, uint64_t* args) {
    uint32_t fd = args[0];
    uint32_t stat_ptr = args[1];

    struct Fd* jos_fd;
    uint8_t filetype = FILETYPE_REGULAR_FILE;
    if (fd < PREOPEN_FD)
        filetype = FILETYPE_CHARACTER_DEVICE;
    else if (fd == PREOPEN_FD)
        filetype = FILETYPE_DIRECTORY;
    else if (fd_lookup(fd - FILE_FD_BASE, &jos_fd) < 0)
        return WASI_EBADF;

    memset(mem(vm, stat_ptr, 24), 0, 24);
    store(vm, stat_ptr, &filetype, sizeof(filetype));
    store_u64(vm, stat_ptr + 8, ~0ULL);
    store_u64(vm, stat_ptr + 16, ~0ULL);
    return WASI_ESUCCESS;
}

static uint64_t
wasi_fd_fdstat_set_flags(VM* vm, uint64_t* args) {
    return WASI_ESUCCESS;
}

static uint64_t
wasi_fd_prestat_get(VM* vm, uint64_t* args) {
    if (args[0] != PREOPEN_FD)
        return WASI_EBADF;

    store_u32(vm, args[1], 0);
    store_u32(vm, args[1] + 4, 1);
    return WASI_ESUCCESS;
}

static uint64_t
wasi_fd_prestat_dir_name(VM* vm, uint64_t* args) {
    if (args[0] != PREOPEN_FD)
        return WASI_EBADF;

    store(vm, args[1], "/", 1);
    return WASI_ESUCCESS;
}

static uint64_t
wasi_fd_read(VM* vm, uint64_t* args) {
    uint32_t fd = args[0];
    WasiIovec* iovs = iovecs(vm, args[1], args[2]);
    uint32_t total = 0;

    if (fd < PREOPEN_FD) {
        uint8_t* buf = mem(vm, iovs[0].buf, iovs[0].len);
        int c;
        while (total < iovs[0].len && (c = sys_getc_nonblocking()) > 0)
            buf[total++] = c;
    } else {
        for (uint32_t idx = 0; idx < args[2]; ++idx) {
            int n = read(fd - FILE_FD_BASE, mem(vm, iovs[idx].buf, iovs[idx].len), iovs[idx].len);
            if (n < 0)
                return WASI_EBADF;
            total += n;
            if ((uint32_t)n < iovs[idx].len)
                break;
        }
    }

    store_u32(vm, args[3], total);
    return WASI_ESUCCESS;
}

static uint64_t
wasi_fd_seek(VM* vm, uint64_t* args) {
    uint32_t fd = args[0];
    int64_t offset = args[1];
    uint8_t whence = args[2];

    struct Fd* jos_fd;
    struct Stat st;
    if (fd < FILE_FD_BASE || fd_lookup(fd - FILE_FD_BASE, &jos_fd) < 0 || fstat(fd - FILE_FD_BASE, &st) < 0)
        return WASI_EBADF;

    if (whence == WHENCE_CUR)
        offset += jos_fd->fd_offset;
    else if (whence == WHENCE_END)
        offset += st.st_size;

    if (offset < 0 || seek(fd - FILE_FD_BASE, offset) < 0)
        return WASI_EINVAL;

    store_u64(vm, args[3], offset);
    return WASI_ESUCCESS;
}

static uint64_t
wasi_fd_write(VM* vm, uint64_t* args) {
    uint32_t fd = args[0];
    if (fd != 1 && fd != 2)
        return WASI_EBADF;

    WasiIovec* iovs = iovecs(vm, args[1], args[2]);
    uint32_t written = 0;
    for (uint32_t idx = 0; idx < args[2]; ++idx) {
        sys_cputs_serial((const char*)mem(vm, iovs[idx].buf, iovs[idx].len), iovs[idx].len);
        written += iovs[idx].len;
    }

    store_u32(vm, args[3], written);
    return WASI_ESUCCESS;
}

static uint64_t
wasi_path_create_directory(VM* vm, uint64_t* args) {
    return WASI_ENOSYS; // not needen for now
}

static uint64_t
wasi_path_open(VM* vm, uint64_t* args) {
    uint32_t path_len = args[3];
    uint16_t oflags = args[4];

    char path[MAXPATHLEN];
    CHECK(path_len + 2 <= sizeof(path), "wasi: path is too long\n");
    path[0] = '/';
    memcpy(path + 1, mem(vm, args[2], path_len), path_len);
    path[path_len + 1] = '\0';

    int mode = O_RDONLY;
    if (oflags & OFLAGS_CREAT)
        mode = O_RDWR | O_CREAT;
    if (oflags & OFLAGS_TRUNC)
        mode |= O_TRUNC;

    int fd = open(path, mode);
    if (fd < 0)
        return WASI_ENOENT;

    store_u32(vm, args[8], fd + FILE_FD_BASE);
    return WASI_ESUCCESS;
}

static uint64_t
wasi_path_remove_directory(VM* vm, uint64_t* args) {
    return WASI_ENOSYS;
}

static uint64_t
wasi_path_rename(VM* vm, uint64_t* args) {
    return WASI_ENOSYS;
}

static uint64_t
wasi_path_unlink_file(VM* vm, uint64_t* args) {
    return WASI_ENOSYS;
}

static uint64_t
wasi_poll_oneoff(VM* vm, uint64_t* args) {
    uint32_t in_ptr = args[0];
    uint32_t out_ptr = args[1];

    uint64_t timeout = load_u64(vm, in_ptr + 24);
    uint16_t flags;
    memcpy(&flags, mem(vm, in_ptr + 40, sizeof(flags)), sizeof(flags));
    uint64_t deadline = flags & 1 ? timeout : now_ns() + timeout;
    while (now_ns() < deadline)
        sys_yield();

    memset(mem(vm, out_ptr, 32), 0, 32);
    store_u64(vm, out_ptr, load_u64(vm, in_ptr));
    store_u32(vm, args[3], 1);
    return WASI_ESUCCESS;
}

static uint64_t
wasi_proc_exit(VM* vm, uint64_t* args) {
    jit_print_fallbacks();
    exit();
    return WASI_ESUCCESS;
}

// clang-format off
static const HostFunc wasi_funcs[] = {
    {"wasi_snapshot_preview1", "args_get", wasi_args_get},
    {"wasi_snapshot_preview1", "args_sizes_get", wasi_args_sizes_get},
    {"wasi_snapshot_preview1", "environ_get", wasi_environ_get},
    {"wasi_snapshot_preview1", "environ_sizes_get", wasi_environ_sizes_get},
    {"wasi_snapshot_preview1", "clock_time_get", wasi_clock_time_get},
    {"wasi_snapshot_preview1", "fd_close", wasi_fd_close},
    {"wasi_snapshot_preview1", "fd_fdstat_get", wasi_fd_fdstat_get},
    {"wasi_snapshot_preview1", "fd_fdstat_set_flags", wasi_fd_fdstat_set_flags},
    {"wasi_snapshot_preview1", "fd_prestat_get", wasi_fd_prestat_get},
    {"wasi_snapshot_preview1", "fd_prestat_dir_name", wasi_fd_prestat_dir_name},
    {"wasi_snapshot_preview1", "fd_read", wasi_fd_read},
    {"wasi_snapshot_preview1", "fd_seek", wasi_fd_seek},
    {"wasi_snapshot_preview1", "fd_write", wasi_fd_write},
    {"wasi_snapshot_preview1", "path_create_directory", wasi_path_create_directory},
    {"wasi_snapshot_preview1", "path_open", wasi_path_open},
    {"wasi_snapshot_preview1", "path_remove_directory", wasi_path_remove_directory},
    {"wasi_snapshot_preview1", "path_rename", wasi_path_rename},
    {"wasi_snapshot_preview1", "path_unlink_file", wasi_path_unlink_file},
    {"wasi_snapshot_preview1", "poll_oneoff", wasi_poll_oneoff},
    {"wasi_snapshot_preview1", "proc_exit", wasi_proc_exit},
};
// clang-format on

static bool
name_eq(const char* str, const uint8_t* name, uint32_t len) {
    return strlen(str) == len && !memcmp(str, name, len);
}

HostFn
wasi_lookup(const uint8_t* module, uint32_t module_len, const uint8_t* name, uint32_t name_len) {
    for (size_t idx = 0; idx < sizeof(wasi_funcs) / sizeof(wasi_funcs[0]); ++idx) {
        const HostFunc* func = &wasi_funcs[idx];
        if (name_eq(func->module, module, module_len) && name_eq(func->name, name, name_len))
            return func->fn;
    }
    return NULL;
}
