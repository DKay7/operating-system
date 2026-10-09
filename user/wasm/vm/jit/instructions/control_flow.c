#include <inc/lib.h>

#include "parser/reader.h"
#include "vm/jit/jit.h"
#include "vm/jit/x86_64.h"
#include "vm/label.h"
#include "vm/stack.h"
#include "vm/vm.h"

static void
open_label(JitCtx* jit, LabelKind kind) {
    Label label;
    label_init(&label, kind, jit->func, &jit->code, jit->height);
    label_push(&jit->labels, label);
}

static void
jit_jump(JitCtx* jit, Cond cond, size_t target) {
    size_t at = 0;
    if (cond == UNCOND)
        at = x86_jmp(jit->buf);
    else
        at = x86_jcc(jit->buf, cond);

    JitFixup fix = {.at = at, .target = target};
    jit->fixups[jit->nfixups++] = fix;
}

static void
emit_branch(JitCtx* jit, uint32_t depth) {
    Label* label = label_get(&jit->labels, depth);
    bool is_loop = label->kind == LABEL_LOOP;
    uint8_t arity = is_loop ? 0 : label->arity;

    if (arity != 0)
        x86_mov_r64_m64(jit->buf, RAX, RBX, -8);

    int32_t shift = ((int32_t)label->height - (int32_t)jit->height) * (int32_t)sizeof(uint64_t);
    if (shift)
        x86_add_r64_imm32(jit->buf, RBX, shift);

    if (arity)
        jit_wasm_push_raw(jit, RAX);

    // jump on continuatiuon, check interpreter for details
    jit_jump(jit, UNCOND, is_loop ? label->start : label->end);
}

bool
jit_block(JitCtx* jit) {
    open_label(jit, LABEL_BLOCK);
    return true;
}

bool
jit_loop(JitCtx* jit) {
    open_label(jit, LABEL_LOOP);
    return true;
}

bool
jit_br(JitCtx* jit) {
    emit_branch(jit, read_u32_leb(&jit->code));
    return true;
}

bool
jit_br_if(JitCtx* jit) {
    uint32_t depth = read_u32_leb(&jit->code);

    jit_wasm_pop(jit, RAX);

    x86_test_r32_r32(jit->buf, RAX, RAX);
    // offset points to next instruction
    jit_jump(jit, CC_E, jit->code.offset); 
    emit_branch(jit, depth);
    return true;
}

bool
jit_return(JitCtx* jit) {
    emit_branch(jit, jit->labels.top - 1);
    return true;
}

bool
jit_if(JitCtx* jit) {
    jit_wasm_pop(jit, RAX);

    open_label(jit, LABEL_IF);
    Label* label = label_get(&jit->labels, 0);

    x86_test_r32_r32(jit->buf, RAX, RAX);
    jit_jump(jit, CC_E, label->else_start ? label->else_start : label->end);
    return true;
}

bool
jit_else(JitCtx* jit) {
    Label* label = label_get(&jit->labels, 0);
    jit_jump(jit, UNCOND, label->end);
    jit->height = label->height;
    return true;
}

bool
jit_end(JitCtx* jit) {
    Label label = label_pop(&jit->labels);
    jit->height = label.height + label.arity;

    if (jit->labels.top == 0)
        jit_emit_epilogue(jit);
    return true;
}

static uint64_t*
jit_call_helper(CallCtx* ctx, uint64_t* sp, uint64_t func_idx) {
    jit_set_stack_top(ctx, sp);
    interp_call(ctx->vm, func_idx);
    return jit_stack_top(ctx);
}

bool
jit_call(JitCtx* jit) {
    uint32_t func_idx = read_u32_leb(&jit->code);
    Module* module = jit->vm->module;
    FuncType* type = &module->types[module->funcs[func_idx].type_idx];

    jit_emit_runtime_call(jit, jit_call_helper, func_idx);

    jit->height = jit->height - type->nparam + type->nresult;
    return true;
}

bool
jit_br_table(JitCtx* jit) {
    uint32_t count = read_u32_leb(&jit->code);
    jit_wasm_pop(jit, RDX);
    
    // may be done via jump table
    for (uint32_t idx = 0; idx < count; ++idx) {
        uint32_t depth = read_u32_leb(&jit->code);

        x86_mov_r64_imm64(jit->buf, RCX, idx);
        x86_cmp_r32_r32(jit->buf, RDX, RCX);
        size_t skip = x86_jcc(jit->buf, CC_NE);

        emit_branch(jit, depth);
        x86_patch_rel32(jit->buf, skip, jit->buf->len);
    }

    emit_branch(jit, read_u32_leb(&jit->code));
    return true;
}

bool
jit_call_indirect(JitCtx* jit) {
    return false;
}
