#include "cc.h"

// @TODO: refactor
static FILE* s_output;

#define print(...) fprintf(s_output, __VA_ARGS__)

#define println(...)                    \
    do {                                \
        fprintf(s_output, __VA_ARGS__); \
        fprintf(s_output, "\n");        \
    } while (0)

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
    case ND_ADD:
    case ND_SUB: {
        gen_ir_expr(func, node->rhs);

        // push rax
        IRx86* ir = new_ir(OP_PUSH);
        target_as_reg(&(ir->lhs), "rax");
        vector_push_back(IR*, func->irs, ir);

        gen_ir_expr(func, node->lhs);

        // pop rdi
        ir = new_ir(OP_POP);
        target_as_reg(&(ir->lhs), "rdi");
        vector_push_back(IR*, func->irs, ir);

        // add op
        ir = new_ir(node->kind == ND_ADD ? OP_ADD : OP_SUB);
        target_as_reg(&(ir->lhs), "rax");
        target_as_reg(&(ir->rhs), "rdi");
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

Vector* gen_x86_ir(Obj* prog)
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

    return funcs;
}

// static void emit

// @TODO: move to other file

void gen_x86(Vector* funcs, char* asmname)
{
    s_output = fopen(asmname, "w");

    println("  .intel_syntax noprefix");

    for (int i = 0; i < funcs->len; ++i) {
        IRx86Func* fn = vector_at(IRx86Func, funcs, i);
        println("  .text");
        if (!fn->isStatic) {
            println("  .global %s", fn->name);
        }
        println("%s:", fn->name);

        // Prologue
        // println("  push %%rbp");
        // println("  mov %%rsp, %%rbp");
        // println("  sub $%d, %%rsp", fn->stackSize);

        for (int i = 0; i < fn->irs->len; ++i) {
            IRx86* ir = *vector_at(IRx86*, fn->irs, i);
            switch (ir->opCode) {
            case OP_RET:
                println("  ret");
                break;
            case OP_MOV:
                assert(ir->lhs.kind == TARGET_REG);
                assert(ir->rhs.kind == TARGET_IMM);
                println("  mov %s, %lld", ir->lhs.name, ir->rhs.imm);
                break;
            case OP_ADD:
                println("  add %s, %s", ir->lhs.name, ir->rhs.name);
                break;
            case OP_SUB:
                println("  sub %s, %s", ir->lhs.name, ir->rhs.name);
                break;
            case OP_PUSH:
                println("  push %s", ir->lhs.name);
                break;
            case OP_POP:
                println("  pop %s", ir->lhs.name);
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

    fclose(s_output);
    s_output = NULL;
}
