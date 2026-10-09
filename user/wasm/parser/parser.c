#include "parser/parser.h"
#include "common/check.h"
#include "inc/assert.h"
#include "inc/types.h"
#include "inc/lib.h"
#include "parser/reader.h"
#include "common/wasm_types.h"
#include "common/opcodes.h"
#include <stdint.h>

// spec i used:
// https://webassembly.github.io/spec/versions/core/WebAssembly-1.0.pdf

int check_magic(Parser*);
int check_version(Parser* parser);
int read_section(Parser* parser);

void parse_functype(Parser* parser, FuncType* out);
void parse_vec_valtypes(Parser* parser, uint32_t* out_len, ValueType** out_data);
void parse_global(Parser* parser, Global* out);
void parse_export(Parser* parser, Export* out);
void parse_func(Parser* parser, Func* out);
static void parse_blocks(Func* func);
void parse_expression(Parser* parser, Expression* out);
void parse_import(Parser* parser, Func* out);
void parse_data(Parser* parser, Data* out);
void parse_elem(Parser* parser, Elem* out);
int parse_data_count_section(Parser* parser, uint32_t sec_size);

int parse_custom_section(Parser*, uint32_t sec_size);
int parse_type_section(Parser*, uint32_t sec_size);
int parse_import_section(Parser* parser, uint32_t sec_size);
int parse_function_section(Parser* parser, uint32_t sec_size);
int parse_table_section(Parser* parser, uint32_t sec_size);
int parse_memory_section(Parser* parser, uint32_t sec_size);
int parse_global_section(Parser* parser, uint32_t sec_size);
int parse_export_section(Parser* parser, uint32_t sec_size);
int parse_start_section(Parser* parser, uint32_t sec_size);
int parse_element_section(Parser* parser, uint32_t sec_size);
int parse_code_section(Parser* parser, uint32_t sec_size);
int parse_data_section(Parser* parser, uint32_t sec_size);


void parser_init(Parser* parser, const uint8_t* buf, size_t size);

Parser*
parse_wasm(const uint8_t* buf, size_t size) {

    Parser* parser = balloc(1, sizeof(Parser));
    parser_init(parser, buf, size);

    int ok = 0;
    ok = check_magic(parser);

    CHECK(ok, "wrong magic");

    ok = check_version(parser);
    CHECK(ok, "wrong version");

    // sections array
    while (reader_remaining(&parser->reader) > 0)
        read_section(parser);

    // TODO: check code section is presented...

    return parser;
}

void
parser_init(Parser* parser, const uint8_t* buf, size_t size) {
    memset(parser, 0, sizeof(*parser));
    reader_init(&parser->reader, buf, size);
}

static bool
is_valtype(uint8_t type) {
    return type == i32 || type == i64 || type == f32 || type == f64;
}


int
check_magic(Parser* parser) {
    const char magic[] = {0, 'a', 's', 'm'};

    return reader_expect(&parser->reader, magic, sizeof(magic));
}

int
check_version(Parser* parser) {
    uint32_t version = read_u32_le(&parser->reader);
    return reader_ok(&parser->reader) && version == 1;
}

int
read_section(Parser* parser) {
    uint8_t sec_id = read_u8(&parser->reader);
    uint32_t sec_size = read_u32_leb(&parser->reader);
    CHECK(reader_ok(&parser->reader), "wrong section header");

    // custom sections may be anywhere, as data count section
    if (sec_id != 0 && sec_id != 12) {
        CHECK(sec_id > parser->last_section_id, "sections out of order");
        parser->last_section_id = sec_id;
    }

    size_t sec_start = parser->reader.offset;

    int ret = 0;
    switch (sec_id) {
    case 0:
        ret = parse_custom_section(parser, sec_size);
        break;
    case 1:
        ret = parse_type_section(parser, sec_size);
        break;
    case 2:
        ret = parse_import_section(parser, sec_size);
        break;
    case 3:
        ret = parse_function_section(parser, sec_size);
        break;
    case 4:
        ret = parse_table_section(parser, sec_size);
        break;
    case 5:
        ret = parse_memory_section(parser, sec_size);
        break;
    case 6:
        ret = parse_global_section(parser, sec_size);
        break;
    case 7:
        ret = parse_export_section(parser, sec_size);
        break;
    case 8:
        ret = parse_start_section(parser, sec_size);
        break;
    case 9:
        ret = parse_element_section(parser, sec_size);
        break;
    case 10:
        ret = parse_code_section(parser, sec_size);
        break;
    case 11:
        ret = parse_data_section(parser, sec_size);
        break;
    case 12:
        ret = parse_data_count_section(parser, sec_size);
        break;

    default:
        CHECK(false, "unsupported section id = %d", sec_id);
    }

    // the section must be read exactly to its declared size
    CHECK(reader_ok(&parser->reader), "wrong section contents");
    CHECK(parser->reader.offset - sec_start == sec_size, "section size mismatch");

    return ret;
}


int
parse_custom_section(Parser* parser, uint32_t sec_size) {
    // name and contents are not needed, skip the whole section
    CHECK(reader_skip(&parser->reader, sec_size), "wrong custom section");
    return 0;
}

int
parse_type_section(Parser* parser, uint32_t sec_size) {
    uint32_t vec_len = read_vec_len(&parser->reader);
    FuncType* ft_array = balloc(vec_len, sizeof(*ft_array));
    CHECK(ft_array != NULL, "Out of memory");

    for (uint32_t idx = 0; idx < vec_len; ++idx) {
        parse_functype(parser, &ft_array[idx]);
    }

    parser->module.types = ft_array;
    parser->module.ntypes = vec_len;

    return 0;
}

void
parse_functype(Parser* parser, FuncType* out) {
    const char expected[] = {0x60};
    bool ok = reader_expect(&parser->reader, expected, sizeof(expected));
    CHECK(ok, "wrong function type");

    parse_vec_valtypes(parser, &out->nparam, &out->param_types);
    parse_vec_valtypes(parser, &out->nresult, &out->result_types);
}

void
parse_vec_valtypes(Parser* parser, uint32_t* out_len, ValueType** out_data) {
    uint32_t len = read_vec_len(&parser->reader);
    *out_len = len;
    *out_data = (ValueType*)read_bytes(&parser->reader, len);
    CHECK(reader_ok(&parser->reader), "wrong vector of value types");

    for (uint32_t idx = 0; idx < len; ++idx)
        CHECK(is_valtype((*out_data)[idx]), "unsupported value type");
}

int
parse_import_section(Parser* parser, uint32_t sec_size) {
    uint32_t vec_len = read_vec_len(&parser->reader);
    Func* import_arr = balloc(vec_len, sizeof(Func));

    CHECK(import_arr, "OOM");

    for (uint32_t idx = 0; idx < vec_len; ++idx) {
        parse_import(parser, &import_arr[idx]);
    }
    CHECK(reader_ok(&parser->reader), "wrong import section");

    parser->module.funcs = import_arr;
    parser->module.nfuncs = vec_len;

    return 0;
}

void
parse_import(Parser* parser, Func* out) {
    uint32_t module_name_len = 0;
    const uint8_t* module_name = read_name(&parser->reader, &module_name_len);

    uint32_t func_name_len = 0;
    const uint8_t* func_name = read_name(&parser->reader, &func_name_len);

    uint8_t kind = read_u8(&parser->reader);
    CHECK(kind == 0, "Only function imports are supported for now"); // TODO :)
    uint32_t type_idx = read_u32_leb(&parser->reader);

    out->imported = true;

    out->import_module = module_name;
    out->import_module_len = module_name_len;

    out->import_name = func_name;
    out->import_name_len = func_name_len;

    out->type_idx = type_idx;
}

int
parse_function_section(Parser* parser, uint32_t sec_size) {

    Module* module = &parser->module;
    uint32_t vec_len = read_vec_len(&parser->reader);
    Func* funcs = balloc(module->nfuncs + vec_len, sizeof(Func));
    CHECK(funcs, "OOM");
    memcpy(funcs, module->funcs, module->nfuncs * sizeof(Func));

    for (uint32_t idx = 0; idx < vec_len; ++idx) {
        funcs[module->nfuncs++].type_idx = read_u32_leb(&parser->reader);
    }
    CHECK(reader_ok(&parser->reader), "wrong function section");

    module->funcs = funcs;

    return 0;
}

int
parse_table_section(Parser* parser, uint32_t sec_size) {
    uint32_t vec_len = read_vec_len(&parser->reader);
    CHECK(vec_len <= 1, "only one table is allowed");
    if (vec_len == 0)
        return 0;

    uint8_t elemtype = read_u8(&parser->reader);
    CHECK(elemtype == 0x70, "only funcref tables are supported");

    uint8_t type = read_u8(&parser->reader);
    CHECK(type == 0x00 || type == 0x01, "wrong type of limit");

    parser->module.table_min = read_u32_leb(&parser->reader);
    parser->module.table_max = parser->module.table_min;
    if (type == 0x01)
        parser->module.table_max = read_u32_leb(&parser->reader);

    CHECK(reader_ok(&parser->reader), "wrong table section");
    parser->module.has_table = true;

    return 0;
}

int
parse_memory_section(Parser* parser, uint32_t sec_size) {

    // max one memory
    uint32_t vec_len = read_vec_len(&parser->reader);
    CHECK(vec_len <= 1, "only one memory is allowed");
    if (vec_len == 0)
        return 0;

    uint8_t type = read_u8(&parser->reader);
    CHECK(type == 0x00 || type == 0x01, "wrong type of limit");

    // min value is always presented
    parser->module.mem_min = read_u32_leb(&parser->reader);
    parser->module.mem_max = 0x10000; // no max

    if (type == 0x01)
        parser->module.mem_max = read_u32_leb(&parser->reader);

    CHECK(reader_ok(&parser->reader), "wrong memory section");
    parser->module.has_memory = true;

    return 0;
}

int
parse_global_section(Parser* parser, uint32_t sec_size) {

    uint32_t vec_len = read_vec_len(&parser->reader);
    Global* globals_arr = balloc(vec_len, sizeof(*globals_arr));
    CHECK(globals_arr, "OOM");

    for (uint32_t idx = 0; idx < vec_len; ++idx)
        parse_global(parser, &globals_arr[idx]);

    parser->module.globals = globals_arr;
    parser->module.nglobals = vec_len;

    return 0;
}

void
parse_global(Parser* parser, Global* out) {
    out->valtype = read_u8(&parser->reader);
    out->mut = read_u8(&parser->reader);
    CHECK(is_valtype(out->valtype), "unsupported global type");
    CHECK(out->mut == type_const || out->mut == type_var, "wrong global mutability");
    parse_expression(parser, &out->init);
}

int
parse_export_section(Parser* parser, uint32_t sec_size) {
    uint32_t vec_len = read_vec_len(&parser->reader);
    Export* export_arr = balloc(vec_len, sizeof(*export_arr));
    CHECK(export_arr, "OOM");

    for (uint32_t idx = 0; idx < vec_len; ++idx)
        parse_export(parser, &export_arr[idx]);
    CHECK(reader_ok(&parser->reader), "wrong export section");

    parser->module.exports = export_arr;
    parser->module.nexports = vec_len;

    return 0;
}


void
parse_export(Parser* parser, Export* out) {
    out->name = read_name(&parser->reader, &out->name_len);
    out->kind = read_u8(&parser->reader);
    out->index = read_u32_leb(&parser->reader);
}

int
parse_start_section(Parser* parser, uint32_t sec_size) {
    parser->module.start_func = read_u32_leb(&parser->reader);
    parser->module.has_start = true;
    return 0;
}
int
parse_element_section(Parser* parser, uint32_t sec_size) {
    uint32_t vec_len = read_vec_len(&parser->reader);
    Elem* elem_arr = balloc(vec_len, sizeof(Elem));
    CHECK(elem_arr, "OOM");

    for (uint32_t idx = 0; idx < vec_len; ++idx) {
        parse_elem(parser, &elem_arr[idx]);
    }
    CHECK(reader_ok(&parser->reader), "wrong element section");

    parser->module.elems = elem_arr;
    parser->module.nelems = vec_len;

    return 0;
}

void
parse_elem(Parser* parser, Elem* out) {
    uint32_t flags = read_u32_leb(&parser->reader);
    CHECK(flags == 0, "only active elements of table 0 are supported\n");
    parse_expression(parser, &out->offset);

    out->len = read_vec_len(&parser->reader);
    out->funcs = balloc(out->len, sizeof(uint32_t));
    CHECK(out->funcs, "OOM");

    for (uint32_t idx = 0; idx < out->len; ++idx) {
        out->funcs[idx] = read_u32_leb(&parser->reader);
    }
}

int
parse_code_section(Parser* parser, uint32_t sec_size) {
    uint32_t vec_len = read_vec_len(&parser->reader);
    CHECK(reader_ok(&parser->reader), "wrong code section");
    // function section gives types, code section gives bodies, one per function

    uint32_t nbodies = 0;
    for (uint32_t idx = 0; idx < parser->module.nfuncs; ++idx) {
        if (parser->module.funcs[idx].imported)
            continue;
        parse_func(parser, &parser->module.funcs[idx]);
        ++nbodies;
    }
    CHECK(nbodies == vec_len, "function and code section sizes differ");

    return 0;
}

#define MAX_LOCALS 50000
void
parse_func(Parser* parser, Func* out) {
    uint32_t size = read_u32_leb(&parser->reader);
    Reader body;
    reader_sub(&parser->reader, size, &body);
    CHECK(reader_ok(&body), "wrong function size");

    // count locals to allocate the array
    Reader locals_start = body;
    uint32_t ngroups = read_vec_len(&body);
    uint32_t nlocals = 0;
    for (uint32_t group = 0; group < ngroups; ++group) {
        uint32_t count = read_u32_leb(&body);
        read_u8(&body);
        CHECK(count <= MAX_LOCALS - nlocals, "too many locals");
        nlocals += count;
    }
    CHECK(reader_ok(&body), "wrong locals");

    out->nlocals = nlocals;
    out->local_types = balloc(nlocals, sizeof(ValueType));
    CHECK(out->local_types, "OOM");

    // xpand (count, type) groups into one type per local
    body = locals_start;
    ngroups = read_vec_len(&body);
    uint32_t local_idx = 0;
    for (uint32_t group = 0; group < ngroups; ++group) {
        uint32_t count = read_u32_leb(&body);
        uint8_t type = read_u8(&body);
        CHECK(is_valtype(type), "unsupported local type");

        for (uint32_t idx = 0; idx < count; ++idx)
            out->local_types[local_idx++] = type;
    }

    out->code = body.buf + body.offset;
    out->code_size = reader_remaining(&body);
    CHECK(out->code_size > 0 && out->code[out->code_size - 1] == 0x0B, "function body must end with 'end'");

    parse_blocks(out);
}

#define MAX_BLOCK_DEPTH 1024
static void
parse_blocks(Func* func) {
    func->block_end = balloc(func->code_size, sizeof(uint32_t));
    func->block_else = balloc(func->code_size, sizeof(uint32_t));
    CHECK(func->block_end && func->block_else, "OOM");
    memset(func->block_end, 0, func->code_size * sizeof(uint32_t));
    memset(func->block_else, 0, func->code_size * sizeof(uint32_t));

    // offsets of the opcodes of currently open blocks
    uint32_t open[MAX_BLOCK_DEPTH];
    uint32_t depth = 0;
    bool function_end = false;

    Reader code;
    reader_init(&code, func->code, func->code_size);

    while (reader_remaining(&code) > 0) {
        uint32_t offset = code.offset;
        uint8_t opcode = read_u8(&code);

        switch (opcode) {
        case OP_BLOCK:
        case OP_LOOP:
        case OP_IF:
            CHECK(depth < MAX_BLOCK_DEPTH, "blocks are nested too deep\n");
            open[depth++] = offset;
            break;

        case OP_ELSE:
            CHECK(depth > 0 && func->code[open[depth - 1]] == OP_IF, "else without if\n");
            func->block_else[open[depth - 1]] = code.offset;
            break;

        case OP_END:
            // end of the function body: must be the last byte
            if (depth == 0) {
                CHECK(code.offset == func->code_size, "code after the end of function\n");
                function_end = true;
                break;
            }
            func->block_end[open[--depth]] = code.offset;
            break;
        }

        // also rejects unknown (unsupported) opcodes
        skip_immediates(opcode, &code);
    }

    CHECK(reader_ok(&code) && function_end, "block without end\n");
}

int
parse_data_section(Parser* parser, uint32_t sec_size) {
    uint32_t vec_len = read_vec_len(&parser->reader);
    Data* data_arr = balloc(vec_len, sizeof(Data));
    CHECK(data_arr, "OOM");

    for (uint32_t idx = 0; idx < vec_len; ++idx) {
        parse_data(parser, &data_arr[idx]);
    }
    CHECK(reader_ok(&parser->reader), "wrong data section");

    parser->module.data = data_arr;
    parser->module.ndata = vec_len;

    return 0;
}

int
parse_data_count_section(Parser* parser, uint32_t sec_size) {
    read_u32_leb(&parser->reader);
    return 0;
}

void
parse_data(Parser* parser, Data* out) {
    uint8_t memidx = read_u8(&parser->reader);
    CHECK(memidx == 0, "other data types are not supported\n");
    parse_expression(parser, &out->offset);

    out->len = read_vec_len(&parser->reader);
    out->bytes = read_bytes(&parser->reader, out->len);
}

void
parse_expression(Parser* parser, Expression* out) {
    Reader* reader = &parser->reader;
    out->instructions = reader->buf + reader->offset;

    uint8_t opcode = read_u8(reader);
    switch (opcode) {
    case OP_I32_CONST:
        read_i32_leb(reader);
        break;
    case OP_I64_CONST:
        read_i64_leb(reader);
        break;
    case OP_GLOBAL_GET:
        read_u32_leb(reader);
        break;
    default: // f32, f64 are not supported
        CHECK(false, "wrong constant expression");
    }

    CHECK(read_u8(reader) == 0x0B && reader_ok(reader), "constant expression must end with 'end'");
    out->len = reader->buf + reader->offset - out->instructions;
}
