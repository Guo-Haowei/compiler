#include "cc.h"

static IRx86* new_ir(int op)
{
    IRx86* ir = calloc(1, sizeof(IRx86));
    ir->opCode = op;
    return ir;
}

static void target_as_imm(Target* target, int64_t val)
{
    target->kind = TARGET_IMM;
    target->imm = val;
}

static void target_as_reg(Target* target, char* reg)
{
    target->kind = TARGET_REG;
    target->name = reg;
}

// static void target_as_label(Target* target, char* label)
// {
//     target->kind = TARGET_LABEL;
//     target->name = label;
// }

static void gen_ir_expr(IRx86Func* func, Node* node)
{
    switch (node->kind) {
    case ND_NUM: {
        IRx86* ir = new_ir(OP_MOV);
        target_as_reg(&(ir->lhs), "rax");
        target_as_imm(&(ir->rhs), node->val);
        vector_push_back(IR*, func->irs, ir);
        return;
    }
    case ND_CAST:
        // TODO: impl cast
        gen_ir_expr(func, node->lhs);
        return;
    default:
        error("node %d not implemented\n", node->kind);
        break;
    }
}

static void gen_ir_stmt(IRx86Func* func, Node* node)
{
    switch (node->kind) {
    case ND_BLOCK:
        for (Node* stmt = node->body; stmt; stmt = stmt->next) {
            gen_ir_stmt(func, stmt);
        }
        return;
    case ND_RETURN: {
        IRx86* ir = new_ir(OP_RET);
        if (node->lhs) {
            gen_ir_expr(func, node->lhs);
        }
        vector_push_back(IR*, func->irs, ir);
        return;
    }
    default:
        error("node %d not implemented\n", node->kind);
        break;
    }
}

IRx86Func* gen_x86_ir(Obj* prog)
{
    Vector* funcs = vector_new(sizeof(IRx86Func), 4);

    for (Obj* fn = prog; fn; fn = fn->next) {
        if (!fn->isFunc || !fn->isDefinition) {
            continue;
        }

        IRx86Func _func;
        vector_push_back(IRx86Func, funcs, _func);
        IRx86Func* func = vector_back(funcs);

        func->name = fn->name;
        func->isStatic = fn->isStatic;
        Vector* irs = vector_new(sizeof(IRx86*), 8);
        func->irs = irs;

        assert(!fn->vaArea);
        assert(!fn->params);

        gen_ir_stmt(func, fn->body);
    }

    // dump_ir(irs, stderr);
    return vector_at(IRx86Func, funcs, 0);
}

// @TODO: move to other file
void gen_x86(IRx86Func* fn)
{
    printf("  .intel_syntax noprefix\n");

    // @TODO refactor
    printf("  .text\n");
    if (!fn->isStatic) {
        printf("  .global %s\n", fn->name);
    }
    printf("%s:\n", fn->name);

    // Prologue
    // println("  push %%rbp");
    // println("  mov %%rsp, %%rbp");
    // println("  sub $%d, %%rsp", fn->stackSize);

    for (int i = 0; i < fn->irs->len; ++i) {
        IRx86* ir = *vector_at(IRx86*, fn->irs, i);
        switch (ir->opCode) {
        case OP_RET:
            printf("  ret\n");
            break;
        case OP_MOV:
            assert(ir->lhs.kind == TARGET_REG);
            assert(ir->rhs.kind == TARGET_IMM);
            printf("  mov %s, %lld\n", ir->lhs.name, ir->rhs.imm);
            break;
        default:
            error("opcode %d is not valid\n", ir->opCode);
            break;
        }
    }

    // Epilogue
    // println(".L.return.%s:", fn->name);
    // println("  mov %%rbp, %%rsp");
    // println("  pop %%rbp");
    // println("  ret");
}
