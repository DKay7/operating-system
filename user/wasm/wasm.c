/* wasm [-j] [-f] <file.wasm> [export] [args...] */
// wasm -j -f doom.wasm _start -iwad doom1.wad -chars block -scaling 2
//
#include <inc/args.h>
#include <inc/lib.h>
#include <inc/x86.h>

#include "common/check.h"
#include "common/io.h"
#include "parser/parser.h"
#include "vm/jit/jit.h"
#include "vm/vm.h"
#include "vm/wasi.h"

typedef struct {
    bool use_jit;
    const char* file;
    const char* func_name; // "main" if not given
    char** args;           // arguments of the function
    int nargs;
    char** wasi_argv;
    int wasi_argc;
} Options;

static uint64_t run(VM* vm, uint32_t func_idx);
static void print_result(VM* vm, FuncType* type, const char* func_name);
static void push_args(VM* vm, FuncType* type, char** args, int nargs);
static Module* load_module(const char* file);
static uint32_t find_function(Module* module, const char* name);
static void parse_options(int argc, char** argv, Options* options);
static void usage(void);

void
umain(int argc, char** argv) {
    Options options;
    parse_options(argc, argv, &options);

    Module* module = load_module(options.file);
    uint32_t func_idx = find_function(module, options.func_name);
    FuncType* type = &module->types[module->funcs[func_idx].type_idx];

    VM* vm = balloc(1, sizeof(VM));
    CHECK(vm, "out of memory\n");
    vm_init(vm, module);

    if (options.use_jit)
        jit_compile_module(vm);

    if (type->nparam == 0)
        wasi_set_args(options.wasi_argc, options.wasi_argv);
    else
        push_args(vm, type, options.args, options.nargs);
    uint64_t cycles = run(vm, func_idx);

    print_result(vm, type, options.func_name);
    printf("cycles: %lu\n", cycles);
    jit_print_fallbacks();
}


static uint64_t
run(VM* vm, uint32_t func_idx) {
    uint64_t start = read_tsc();
    interp_call(vm, func_idx);
    return read_tsc() - start;
}

static void
print_result(VM* vm, FuncType* type, const char* func_name) {
    if (type->nresult == 0)
        return;

    uint64_t result = pop(&vm->stack);
    if (type->result_types[0] == i64)
        printf("%s = %ld\n", func_name, (int64_t)result);
    else
        printf("%s = %d\n", func_name, (int32_t)result);
}

static void
push_args(VM* vm, FuncType* type, char** args, int nargs) {
    CHECK((uint32_t)nargs == type->nparam, "the function takes %u arguments, %d given\n",
          type->nparam, nargs);

    for (int idx = 0; idx < nargs; ++idx) {
        int64_t value = strtol(args[idx], NULL, 0);
        if (type->param_types[idx] == i32)
            push(&vm->stack, (uint32_t)value);
        else
            push(&vm->stack, value);
    }
}

static Module*
load_module(const char* file) {
    size_t size = 0;
    char* buf = read_file(file, &size);
    Parser* parser = parse_wasm((const uint8_t*)buf, size);
    return &parser->module;
}

static uint32_t
find_function(Module* module, const char* name) {
    uint32_t func_idx = 0;
    CHECK(find_export(module, name, &func_idx), "no exported function '%s'\n", name);
    return func_idx;
}

static void
parse_options(int argc, char** argv, Options* options) {
    memset(options, 0, sizeof(*options));

    // stealed from 'ls'
    struct Argstate args;
    argstart(&argc, argv, &args);
    int flag = -1;
    while ((flag = argnext(&args)) >= 0) {
        switch (flag) {
        case 'j':
            options->use_jit = true;
            break;
        case 'f':
            jit_count_fallbacks = true;
            break;
        default:
            usage();
        }
    }

    if (argc < 2)
        usage();

    options->file = argv[1];
    options->func_name = argc > 2 ? argv[2] : "main";
    options->args = argv + 3;
    options->nargs = argc > 3 ? argc - 3 : 0;
    options->wasi_argv = argv + 2;
    options->wasi_argc = argc > 2 ? argc - 2 : 0;
}

static void
usage(void) {
    printf("usage: wasm [-j] [-f] <file.wasm> [export] [args...]\n"
           "  -j  enable jit\n"
           "  -f  count jit fallbacks per opcode\n");
    exit();
}
