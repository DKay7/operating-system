#include "common/check.h"
#include "inc/uefi.h"
#include "parser/reader.h"
#include "vm/interp_call.h"
#include "vm/label.h"
#include "vm/stack.h"
#include "vm/vm.h"
#include <stdint.h>


void
vm_unreachable(CallCtx* ctx) {
    callctx_dump(ctx);
    CHECK(false, "Reached unreachable instruction\n");
}

void
vm_nop(CallCtx* ctx) {
    // punish user for nopping with me
    int idx = 1e9;
    while (idx--);

    (void)ctx;
}

void
vm_block(CallCtx* ctx) {
    Label label;
    label_init(&label, LABEL_BLOCK, ctx->func, &ctx->code, ctx->stack->top);
    label_push(ctx->labels, label);
}

void
vm_loop(CallCtx* ctx) {
    Label label;
    label_init(&label, LABEL_LOOP, ctx->func, &ctx->code, ctx->stack->top);
    label_push(ctx->labels, label);
}

void
vm_if(CallCtx* ctx) {
    uint32_t condition = pop(ctx->stack);

    Label label;
    label_init(&label, LABEL_IF, ctx->func, &ctx->code, ctx->stack->top);
    label_push(ctx->labels, label);

    // if cond == true, goes into if-clause (do nothing here)
    if (condition != 0)
        return;

    // if cond == false, gonna skip the if-part with reader
    // we have 'else', go to it
    if (label.else_start != 0)
        reader_goto(&ctx->code, label.else_start);
    else
        // no else: go to the 'end'
        reader_goto(&ctx->code, label.end - 1);
}

void
vm_else(CallCtx* ctx) {
    // if we are ended up here, that means that if-clause WAS executed
    // if cond == false in if-else-0clause, we jump directly to the instruction
    // right after 'else'
    // so if we met 'else', that means we already executed 'if' and
    // have to skip 'else'-block

    // go to the 'end' op of the 'if'
    Label* if_else = label_get(ctx->labels, 0);
    reader_goto(&ctx->code, if_else->end - 1);
}

void
vm_end(CallCtx* ctx) {
    // end of a block, or of the function body (pops the entry label)
    label_pop(ctx->labels);
}

// used by br, br_if, return
static void
branch(CallCtx* ctx, uint32_t depth) {
    CHECK(depth < ctx->labels->top - ctx->labels_base, "wrong label depth %u\n", depth);

    Label target = *label_get(ctx->labels, depth);

    // keep the result, drop everything the block pushed above it
    uint64_t result = target.arity ? pop(ctx->stack) : 0;
    ctx->stack->top = target.height;

    bool is_loop = target.kind == LABEL_LOOP;
    uint8_t arity = is_loop ? 0 : target.arity;

    if (arity != 0)
        push(ctx->stack, result);

    // drop inner labels
    ctx->labels->top -= depth;

    // spec said to jump to *continuation* of L
    if (target.kind == LABEL_LOOP) {
        reader_goto(&ctx->code, target.start);
    } else {
        // also drop target label itself
        ctx->labels->top -= 1;
        reader_goto(&ctx->code, target.end);
    }
}

void
vm_br(CallCtx* ctx) {
    branch(ctx, read_u32_leb(&ctx->code));
}

void
vm_br_if(CallCtx* ctx) {
    uint32_t depth = read_u32_leb(&ctx->code);
    uint32_t condition = pop(ctx->stack);

    if (condition)
        branch(ctx, depth);
}

void
vm_br_table(CallCtx* ctx) {

    uint32_t target_num = pop(ctx->stack);
    uint32_t count = read_u32_leb(&ctx->code);
    target_num = MIN(target_num, count);
    uint32_t label_id = 0;
    for (uint32_t idx = 0; idx <= target_num; ++idx) {
        label_id = read_u32_leb(&ctx->code);
    }

    branch(ctx, label_id);
}

void
vm_return(CallCtx* ctx) {
    // br to the entry label
    uint32_t entry_label_idx = ctx->labels->top - ctx->labels_base - 1;
    branch(ctx, entry_label_idx);
}

void
vm_call(CallCtx* ctx) {
    // callee takes its args from our stack and pushes the result
    interp_call(ctx->vm, read_u32_leb(&ctx->code));
}

void
vm_call_indirect(CallCtx* ctx) {
    // ~~no vcalls since we are FAST
    // now im doing fckng doom so i am stupid i am stupid
    
    uint32_t type_idx = read_u32_leb(&ctx->code);
    (void)type_idx;
    read_u32_leb(&ctx->code); // table index which is always 0 in wasm1.0
    
    uint32_t vtable_idx = pop(ctx->stack);
    CHECK(vtable_idx < ctx->vm->table_size, "call out of bounds via table");
    uint32_t func_idx = ctx->vm->table[vtable_idx];

    CHECK(func_idx != UINT32_MAX, "calling empty table");

    // todo: probably it would be nice to add a 
    // CHECK(ctx->vm->module->types[type_idx] == 
    //       ctx->vm->module->types[ctx->vm->module->funcs[func_idx].type]);
    // but i'm done of checks
    // you re on your own now
    // someone add a fucking Undefined Behaviour in wasm please
    // we could not compare idxs here since same types may be 
    // uder different indexes in table
    // so one must to write a fair comparator and im done with it, honestly
    // my last brain cell could not afford it
    // probably, i've already wrote a longer comment that a comparator would have
    // but writing commnts is fun
    // writing comparator is not
    
    interp_call(ctx->vm, func_idx);
}
