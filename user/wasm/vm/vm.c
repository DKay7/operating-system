#include <inc/lib.h>

#include "vm/vm.h"
#include "common/check.h"
#include "common/opcodes.h"
#include "parser/reader.h"


static uint64_t
eval_const_expr(VM* vm, Expression* expr);

void
vm_init(VM* vm, Module* module) {
    memset(vm, 0, sizeof(*vm));
    vm->module = module;

    stack_init(&vm->stack, STACK_SIZE);
    label_stack_init(&vm->labels, LABELS_SIZE);

    vm->globals = balloc(module->nglobals, sizeof(uint64_t));
    CHECK(vm->globals, "out of memory\n");
    for (uint32_t idx = 0; idx < module->nglobals; ++idx)
        vm->globals[idx] = eval_const_expr(vm, &module->globals[idx].init);

    if (module->has_memory) {
        vm->mem_size = (uint64_t)module->mem_min * WASM_PAGE_SIZE;
        vm->memory = balloc(vm->mem_size, 1);
        CHECK(vm->memory, "out of memory\n");
    }
}

static uint64_t
eval_const_expr(VM* vm, Expression* expr) {
    Reader reader;
    reader_init(&reader, expr->instructions, expr->len);

    uint8_t opcode = read_u8(&reader);
    switch (opcode) {
    case OP_I32_CONST:
        return (uint32_t)read_i32_leb(&reader);
    case OP_I64_CONST:
        return read_i64_leb(&reader);
    case OP_GLOBAL_GET:
        return vm->globals[read_u32_leb(&reader)];
    default:
        CHECK(false, "wrong constant expression\n");
        return 0;
    }
}

bool
find_export(Module* module, const char* name, uint32_t* func_idx) {
    size_t len = strlen(name);

    for (uint32_t idx = 0; idx < module->nexports; ++idx) {
        Export* export = &module->exports[idx];
        if (export->kind == func && export->name_len == len &&
            memcmp(export->name, name, len) == 0) {
            *func_idx = export->index;
            return true;
        }
    }
    return false;
}
