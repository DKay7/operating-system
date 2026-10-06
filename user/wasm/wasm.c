/* wasm <file.wasm> [export] [args...] -- load, JIT-compile and run a module */

#include <inc/lib.h>
#include "common/io.h"
#include "parser/parser.h"
#include "common/check.h"
#include "vm/vm.h"

void usage();

void
umain(int argc, char** argv) {

    if (argc < 2) {
        usage();
        exit();
    }

    size_t size = 0;
    char* buf = read_file(argv[1], &size);

    Parser* parser = parse_wasm((const uint8_t*)buf, size);

    const char* func_name = argc > 2 ? argv[2] : "main";
    uint32_t func_idx = 0;
    CHECK(find_export(&parser->module, func_name, &func_idx), "no exported function '%s'\n", func_name);

    VM* vm = balloc(1, sizeof(VM));
    CHECK(vm, "out of memory\n");
    vm_init(vm, &parser->module);

    FuncType* func_type = &parser->module.types[parser->module.funcs[func_idx].type_idx];
    CHECK(argc - 3 == (int)func_type->nparam, "'%s' takes %u arguments, %d given\n",
          func_name, func_type->nparam, argc > 3 ? argc - 3 : 0);

    // args go to the value stack
    for (int arg = 3; arg < argc; ++arg)
        push(&vm->stack, strtol(argv[arg], NULL, 0));

    interp_call(vm, func_idx);

    FuncType* type = &parser->module.types[parser->module.funcs[func_idx].type_idx];
    if (type->nresult > 0) {
        if (*type->result_types == i32)
            printf("%s = %d\n", func_name, (int32_t)pop(&vm->stack));
        if (*type->result_types == i64)
            printf("%s = %ld\n", func_name, (int64_t)pop(&vm->stack));
    }
}

void
usage() {
    // TODO: help string on options
    printf("wasm jit usage: wasm <file.wasm> [export] [args...]\n");
}
