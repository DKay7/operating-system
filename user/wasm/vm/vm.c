#include <inc/lib.h>
#include <stdint.h>
#include <string.h>

#include "vm/vm.h"
#include "common/check.h"
#include "common/opcodes.h"
#include "common/wasm_types.h"
#include "parser/reader.h"
#include "vm/wasi.h"


static uint64_t
eval_const_expr(VM* vm, Expression* expr);
static void vm_init_data(VM* vm);
static void vm_init_table(VM* vm);
static void vm_link_imports(VM* vm);

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
        vm->mem_capacity = (uint64_t)MAX(module->mem_min, MIN(module->mem_max, MAX_MEM_PAGES)) * WASM_PAGE_SIZE;
        vm->memory = balloc(vm->mem_capacity, 1);
        CHECK(vm->memory, "out of memory\n");
    }

    vm_init_data(vm);
    vm_init_table(vm);
    vm_link_imports(vm);
}

static void
vm_init_data(VM* vm) {
    for (uint32_t idx = 0; idx < vm->module->ndata; ++idx) {
        Data* data = &vm->module->data[idx];
        uint64_t offset = (uint32_t)eval_const_expr(vm, &data->offset);
        CHECK(offset + data->len <= vm->mem_size, "data segment is out of bounds\n");
        memcpy(vm->memory + offset, data->bytes, data->len);
    }
}

static void
vm_init_table(VM* vm) {
    // CHECK(vm->module->nelems == 0, "tables are not implemented\n");
    // now they fucking are
    
    if (!vm->module->has_table)
        return;
    vm->table_size = vm->module->table_min;
    vm->table = balloc(vm->table_size, sizeof(*vm->table));
    CHECK(vm->table, "out of memory\n");
    
    // means empty
    memset(vm->table, UINT32_MAX, vm->table_size * sizeof(*vm->table));
    
    for (uint32_t idx = 0; idx < vm->module->nelems; ++idx) {
        Elem *cur_elem = &vm->module->elems[idx];
        uint64_t offset = (uint32_t)eval_const_expr(vm, &cur_elem->offset);
        CHECK(offset + cur_elem->len <= vm->table_size, "out of bouds table idx");
        memcpy(vm->table + offset, cur_elem->funcs, cur_elem->len * sizeof(uint32_t));
    }

}

static void
vm_link_imports(VM* vm) {
    for (uint32_t idx = 0; idx < vm->module->nfuncs; ++idx) {
        Func* func = &vm->module->funcs[idx];
        if (!func->imported)
            continue;

        func->host = wasi_lookup(func->import_module, func->import_module_len, func->import_name,
                                 func->import_name_len);
        CHECK(func->host, "unknown import %.*s\n",
              func->import_name_len, func->import_name);
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
