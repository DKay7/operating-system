/* wasm <file.wasm> [export] [args...] -- load, JIT-compile and run a module */

#include <inc/lib.h>
#include "common/io.h"
#include "parser/parser.h"
#include "common/check.h"
#include "vm/vm.h"

void usage();

void
umain(int argc, char **argv) {
    
    if (argc < 2) {
        usage();
        exit();
    }

    size_t size = 0;
    char *buf = read_file(argv[1], &size);

    Parser* parser = parse_wasm((const uint8_t *)buf, size);

    const char* func_name = argc > 2 ? argv[2] : "main";
    uint32_t func_idx = 0;
    CHECK(find_export(&parser->module, func_name, &func_idx), "no exported function '%s'\n", func_name);

    VM* vm = balloc(1, sizeof(VM));
    CHECK(vm, "out of memory\n");
    vm_init(vm, &parser->module);

    FuncType* func_type = &parser->module.types[parser->module.funcs[func_idx].type_idx];
    CHECK(argc - 3 == (int)func_type->nparam, "'%s' takes %u arguments, %d given\n",
          func_name, func_type->nparam, argc > 3 ? argc - 3 : 0);

    // args go to the value stack, as if pushed by a caller
    for (int arg = 3; arg < argc; ++arg)
        push(&vm->stack, (uint32_t)strtol(argv[arg], NULL, 0));

    interp_call(vm, func_idx);

    FuncType* type = &parser->module.types[parser->module.funcs[func_idx].type_idx];
    if (type->nresult > 0)
        printf("%s = %ld\n", func_name, (long)(int32_t)pop(&vm->stack));
    
    /*
     * 3. Parse (parser/reader.c, parser/parser.c) -> struct wasm_module.
     *    Spec: "5 Binary Format". Check magic "\0asm" and version 1, then the
     *    sections in increasing id order (custom ones anywhere).
     *    The input is hostile: every read goes through a bounds-checked reader,
     *    every LEB128 is checked for overlong/overflowing encoding, every
     *    count is checked against limits.h BEFORE allocating, each section
     *    must be consumed exactly to its declared size.
     *
     * 4. Validate (parser/validator.c).
     *    Spec: "3 Validation" and "Appendix: Validation Algorithm".
     *    All indices in range, constant expressions, then type-check every
     *    function body with an operand stack and a control stack.
     *    Reject f32/f64 with WASM_E_UNSUPPORTED. This is the security boundary:
     *    the interpreter and the JIT assume a valid module and only assert().
     *
     * 5. Instantiate (vm/vm.c, vm/memory.c).
     *    Resolve function imports against host functions (exact signature
     *    match), allocate linear memory, globals, table; apply elem/data
     *    segments with bounds checks; run the start function if any.
     *
     * 6. Execute.
     *    First the interpreter (vm/interp.c): simple, it is the reference.
     *    Then the JIT (vm/jit/): one pass over the bytecode, a code template
     *    per opcode, operand stack kept in the native frame.
     *      - W^X (vm/jit/codebuf.c): emit into an RW staging mapping, then
     *        sys_map_region() the pages R+X to WASM_CODE_BASE and unmap staging.
     *      - Memory accesses: explicit check  addr + offset + size <= mem size
     *        (64-bit arithmetic, no overflow), jump to trap otherwise.
     *      - call_indirect: check table index and the callee signature.
     *      - Prologue: check stack depth to turn infinite recursion into a trap.
     *      - Traps (div by zero, INT_MIN / -1, unreachable, ...) call back into
     *        the runtime, which unwinds to vm_invoke() and reports the trap.
     *    Run the tests on both engines and compare results.
     *
     * 7. Output and cleanup.
     *    Find the export, convert args by its signature, invoke, print the
     *    result or the trap name. Unmap memory and code, destroy the arena.
     */
}

void
usage() {
    // TODO: help string on options
    printf("wasm jit usage: wasm <file.wasm> [export] [args...]\n");
}
