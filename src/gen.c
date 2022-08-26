#include "minic.h"

#include <stdio.h>
#include <string.h>

enum {
    I8,
    I16,
    I32,
    I64
};

// @TODO: Support global initializer
// static char* s_argreg8[4] = { "%cl", "%dl", "%r8b", "%r9b" };
// static char* s_argreg16[4] = { "%cx", "%dx", "%r8w", "%r9w" };
// static char* s_argreg32[4] = { "%ecx", "%edx", "%r8d", "%r9d" };
// static char* s_argreg64[4] = { "%rcx", "%rdx", "%r8", "%r9" };
static char* s_argreg8[4];
static char* s_argreg16[4];
static char* s_argreg32[4];
static char* s_argreg64[4];

static int s_depth;
static Obj* s_current_fn;
static FILE* s_output;

// The table for type casts
// static char* s_i32i8 = "movsbl %al, %eax";
// static char* s_i32i16 = "movswl %ax, %eax";
// static char* s_i32i64 = "movsxd %eax, %rax";
static char* s_i32i8;
static char* s_i32i16;
static char* s_i32i64;

// static char* s_cast_table[4][4] = {
//     { NULL, NULL, NULL, s_i32i64 },        // i8
//     { s_i32i8, NULL, NULL, s_i32i64 },     // i16
//     { s_i32i8, s_i32i16, NULL, s_i32i64 }, // i32
//     { s_i32i8, s_i32i16, NULL, NULL },     // i64
// };
static char* s_cast_table[4][4];

static void dummysetup()
{
    s_argreg8[0] = "%cl";
    s_argreg8[1] = "%dl",
    s_argreg8[2] = "%r8b";
    s_argreg8[3] = "%r9b";

    s_argreg16[0] = "%cx";
    s_argreg16[1] = "%dx",
    s_argreg16[2] = "%r8w";
    s_argreg16[3] = "%r9w";

    s_argreg32[0] = "%ecx";
    s_argreg32[1] = "%edx",
    s_argreg32[2] = "%r8d";
    s_argreg32[3] = "%r9d";

    s_argreg64[0] = "%rcx";
    s_argreg64[1] = "%rdx",
    s_argreg64[2] = "%r8";
    s_argreg64[3] = "%r9";

    s_i32i8 = "movsbl %al, %eax";
    s_i32i16 = "movswl %ax, %eax";
    s_i32i64 = "movsxd %eax, %rax";

    s_cast_table[0][3] = s_i32i64;
    s_cast_table[1][0] = s_i32i8;
    s_cast_table[1][3] = s_i32i64;
    s_cast_table[2][0] = s_i32i8;
    s_cast_table[2][1] = s_i32i16;
    s_cast_table[2][3] = s_i32i64;
    s_cast_table[3][0] = s_i32i8;
    s_cast_table[3][1] = s_i32i16;
}

#define println(...)                    \
    {                                   \
        fprintf(s_output, __VA_ARGS__); \
        fprintf(s_output, "\n");        \
    }                                   \
    (void)0

static void push()
{
    println("  push %%rax");
    s_depth++;
}

static void pop(char* arg)
{
    println("  pop %s", arg);
    s_depth--;
}

static void gen_expr(Node* node);

// Load a value from where %rax is pointing to.
static void load(Type* type)
{
    if (type->kind == TY_ARRAY || type->kind == TY_STRUCT) {
        // If it is an array, do not attempt to load a value to the
        // register because in general we can't load an entire array to a
        // register. As a result, the result of an evaluation of an array
        // becomes not the array itself but the address of the array.
        // This is where "array is automatically converted to a pointer to
        // the first element of the array in C" occurs.
        return;
    }

    // When we load a char or a short value to a register, we always
    // extend them to the size of int, so we can assume the lower half of
    // a register always contains a valid value. The upper half of a
    // register for char, short and int may contain garbage. When we load
    // a long value to a register, it simply occupies the entire register.
    switch (type->size) {
    case 1:
        println("  movsbl (%%rax), %%eax");
        break;
    case 2:
        println("  movswl (%%rax), %%eax");
        break;
    case 4:
        println("  movsxd (%%rax), %%rax");
        break;
    case 8:
        println("  mov (%%rax), %%rax");
        break;
    default:
        assert(0);
        break;
    }
}

// Store %rax to an address that the stack top is pointing to.
static void store(Type* type)
{
    pop("%rdi");

    if (type->kind == TY_STRUCT) {
        for (int i = 0; i < type->size; i++) {
            println("  mov %d(%%rax), %%r8b", i);
            println("  mov %%r8b, %d(%%rdi)", i);
        }
        return;
    }

    switch (type->size) {
    case 1:
        println("  mov %%al, (%%rdi)");
        break;
    case 2:
        println("  mov %%ax, (%%rdi)");
        break;
    case 4:
        println("  mov %%eax, (%%rdi)");
        break;
    case 8:
        println("  mov %%rax, (%%rdi)");
        break;
    default:
        assert(0);
        break;
    }
}

// Compute the absolute address of a given node.
// It's an error if a given node does not reside in memory.
static void gen_addr(Node* node)
{
    switch (node->eNodeKind) {
    case ND_VAR:
        if (node->var->isLocal) {
            // local
            println("  lea %d(%%rbp), %%rax", node->var->offset);
        } else {
            // global
            println("  lea %s(%%rip), %%rax", node->var->name);
        }
        return;
    case ND_DEREF:
        gen_expr(node->lhs);
        return;
    case ND_COMMA:
        gen_expr(node->lhs);
        gen_addr(node->rhs);
        return;
    case ND_MEMBER:
        gen_addr(node->lhs);
        println("  add $%d, %%rax", node->member->offset);
        return;
    default:
        break;
    }

    error_tok(node->tok, "not an lvalue");
}

static void gen_cmp_expr(NodeKind eNodeKind, char* di, char* ax)
{
    char* s_cmds[6] = {
        "sete",
        "setne",
        "setl",
        "setle",
        "setg",
        "setge"
    };

    int index = eNodeKind - ND_EQ;
    ASSERT_IDX(index, ARRAY_COUNTER(s_cmds));

    println("  cmp %s, %s", di, ax);
    println("  %s %%al", s_cmds[index]);
    println("  movzb %%al, %%rax");
}

static int get_type_id(Type* ty)
{
    switch (ty->kind) {
    case TY_CHAR:
        return I8;
    case TY_SHORT:
        return I16;
    case TY_INT:
        return I32;
    default:
        return I64;
    }
}

static void gen_cast(Type* from, Type* to)
{
    if (to->kind == TY_VOID) {
        return;
    }

    int t1 = get_type_id(from);
    int t2 = get_type_id(to);
    if (s_cast_table[t1][t2]) {
        println("  %s", s_cast_table[t1][t2]);
    }
}

static void gen_expr(Node* node)
{
    switch (node->eNodeKind) {
    case ND_NULL_EXPR:
        return;
    case ND_NUM:
        println("  mov $%lld, %%rax", node->val);
        return;
    case ND_NEG:
        // @TODO: fix
        gen_expr(node->lhs);
        // if (node->type->size == 8) {
        println("  neg %%rax");
        // } else {
        //     println("  neg %%eax");
        // }
        return;
    case ND_VAR:
    case ND_MEMBER:
        gen_addr(node);
        load(node->type);
        return;
    case ND_DEREF:
        gen_expr(node->lhs);
        load(node->type);
        return;
    case ND_ADDR:
        gen_addr(node->lhs);
        return;
    case ND_MEMZERO:
        // `rep stosb` is equivalent to `memset(%rdi, %al, %rcx)`.
        println("  mov $%d, %%rcx", node->var->type->size);
        println("  lea %d(%%rbp), %%rdi", node->var->offset);
        println("  mov $0, %%al");
        println("  rep stosb");
        return;
    case ND_ASSIGN:
        gen_addr(node->lhs);
        push();
        gen_expr(node->rhs);
        store(node->type);
        return;
    case ND_COMMA:
        gen_expr(node->lhs);
        gen_expr(node->rhs);
        return;
    case ND_FUNCCALL: {
        int argc = node->args->len;

        int regCnt = MIN(argc, 4);
        // first 4 args saved in registers
        for (int i = 0; i < regCnt; ++i) {
            Node* arg = array_at(Node, node->args, i);
            gen_expr(arg);
            push();
        }

        for (int i = regCnt - 1; i >= 0; --i) {
            pop(s_argreg64[i]);
        }

        for (int i = node->args->len - 1; i >= 4; --i) {
            Node* arg = array_at(Node, node->args, i);
            gen_expr(arg);
            push();
        }

        // shadow
        println("  sub $32, %%rsp");
        println("  call %s", node->funcname);

        int stackArgs = MAX(node->args->len - 4, 0);
        int restore = 32 + stackArgs * 8;
        println("  add $%d, %%rsp", restore);
        s_depth -= stackArgs;

        // It looks like the most significant 48 or 56 bits in RAX may
        // contain garbage if a function return type is short or bool/char,
        // respectively. We clear the upper bits here.
        int kind = node->type->kind;
        if (kind == TY_CHAR) {
            println("  movsbl %%al, %%eax");
        } else if (kind == TY_SHORT) {
            println("  movswl %%ax, %%eax");
        }
        return;
    }
    case ND_LOGAND: {
        int c = unique_id();
        gen_expr(node->lhs);
        println("  cmp $0, %%rax");
        println("  je .L.false.%d", c);
        gen_expr(node->rhs);
        println("  cmp $0, %%rax");
        println("  je .L.false.%d", c);
        println("  mov $1, %%rax");
        println("  jmp .L.end.%d", c);
        println(".L.false.%d:", c);
        println("  mov $0, %%rax");
        println(".L.end.%d:", c);
        return;
    }
    case ND_LOGOR: {
        int c = unique_id();
        gen_expr(node->lhs);
        println("  cmp $0, %%rax");
        println("  jne .L.true.%d", c);
        gen_expr(node->rhs);
        println("  cmp $0, %%rax");
        println("  jne .L.true.%d", c);
        println("  mov $0, %%rax");
        println("  jmp .L.end.%d", c);
        println(".L.true.%d:", c);
        println("  mov $1, %%rax");
        println(".L.end.%d:", c);
        return;
    }
    case ND_CAST:
        gen_expr(node->lhs);
        gen_cast(node->lhs->type, node->type);
        return;
    case ND_NOT:
        gen_expr(node->lhs);
        println("  cmp $0, %%rax");
        println("  sete %%al");
        println("  movzx %%al, %%rax");
        return;
    case ND_TERNARY: {
        int c = unique_id();
        gen_expr(node->cond);
        println("  cmp $0, %%rax");
        println("  je .L.else.%d", c);
        gen_expr(node->then);
        println("  jmp .L.end.%d", c);
        println(".L.else.%d:", c);
        gen_expr(node->els);
        println(".L.end.%d:", c);
        return;
    }
    case ND_BITNOT:
        gen_expr(node->lhs);
        println("  not %%rax");
        return;
    default:
        break;
    }

    gen_expr(node->rhs);
    push();
    gen_expr(node->lhs);
    pop("%rdi");

    char *ax = NULL, *di = NULL;
    if (node->lhs->type->kind == TY_LONG || node->lhs->type->base) {
        ax = "%rax";
        di = "%rdi";
    } else {
        ax = "%eax";
        di = "%edi";
    }

    switch (node->eNodeKind) {
    case ND_ADD:
        println("  add %s, %s", di, ax);
        return;
    case ND_SUB:
        println("  sub %s, %s", di, ax);
        return;
    case ND_MUL:
        println("  imul %s, %s", di, ax);
        return;
    case ND_DIV:
    case ND_MOD:
        if (node->lhs->type->size == 8) {
            println("  cqo");
        } else {
            println("  cdq");
        }
        println("  idiv %s", di);
        if (node->eNodeKind == ND_MOD) {
            println("  mov %%rdx, %%rax");
        }
        return;
    case ND_BITAND:
        println("  and %%rdi, %%rax");
        return;
    case ND_BITOR:
        println("  or %%rdi, %%rax");
        return;
    case ND_BITXOR:
        println("  xor %%rdi, %%rax");
        return;
    case ND_EQ:
    case ND_NE:
    case ND_LT:
    case ND_LE:
    case ND_GT:
    case ND_GE:
        gen_cmp_expr(node->eNodeKind, di, ax);
        return;
    case ND_SHL:
        println("  mov %%rdi, %%rcx");
        println("  shl %%cl, %s", ax);
        return;
    case ND_SHR:
        println("  mov %%rdi, %%rcx");
        if (node->type->size == 8) {
            println("  sar %%cl, %s", ax);
        } else {
            println("  sar %%cl, %s", ax);
        }
        return;
    default:
        break;
    }

    error_tok(node->tok, "invalid statement");
}

static void gen_stmt(Node* node)
{
    println("  .loc 1 %d", node->tok->line);

    switch (node->eNodeKind) {
    case ND_IF: {
        int c = unique_id();
        gen_expr(node->cond);
        println("  cmp $0, %%rax");
        println("  je  .L.else.%d", c);
        gen_stmt(node->then);
        println("  jmp .L.end.%d", c);
        println(".L.else.%d:", c);
        if (node->els) {
            gen_stmt(node->els);
        }
        println(".L.end.%d:", c);
        return;
    }
    case ND_EXPR_STMT:
        gen_expr(node->lhs);
        return;
    case ND_RETURN:
        if (node->lhs) {
            gen_expr(node->lhs);
        }
        println("  jmp .L.return.%s", s_current_fn->name);
        return;
    case ND_GOTO:
        println("  jmp %s", node->uniqueLabel);
        return;
    case ND_LABEL:
        println("%s:", node->uniqueLabel);
        gen_stmt(node->lhs);
        return;
    case ND_BLOCK:
        for (Node* n = node->body; n; n = n->next) {
            gen_stmt(n);
        }
        return;
    case ND_FOR: {
        assert(node->brkLabel);
        int c = unique_id();
        if (node->init) {
            gen_stmt(node->init);
        }

        println(".L.begin.%d:", c);

        if (node->cond) {
            gen_expr(node->cond);
            println("  cmp $0, %%rax");
            println("  je %s", node->brkLabel);
        }

        gen_stmt(node->then);
        println("%s:", node->cntLabel);
        if (node->inc) {
            gen_expr(node->inc);
        }
        println("  jmp .L.begin.%d", c);
        println("%s:", node->brkLabel);
        return;
    }
    case ND_SWITCH:
        gen_expr(node->cond);
        for (Node* n = node->caseNext; n; n = n->caseNext) {
            char* reg = (node->cond->type->size == 8) ? "%rax" : "%eax";
            int val = n->val;
            println("  cmp $%d, %s", val, reg);
            println("  je %s", n->label);
        }
        if (node->caseDefault) {
            println("  jmp %s", node->caseDefault->label);
        }
        println("  jmp %s", node->brkLabel);
        gen_stmt(node->then);
        println("%s:", node->brkLabel);
        return;
    case ND_CASE:
        println("%s:", node->label);
        gen_stmt(node->lhs);
        return;
    default:
        break;
    }

    error_tok(node->tok, "invalid statement");
}

// Assign offsets to local variables.
static void assign_lvar_offsets(Obj* prog)
{
    for (Obj* fn = prog; fn; fn = fn->next) {
        if (!fn->isFunc) {
            continue;
        }

        int offset = 0;
        for (Obj* var = fn->locals; var; var = var->next) {
            offset += var->type->size;
            offset = ALIGN(offset, var->type->align);
            var->offset = -offset;
        }
        fn->stackSize = ALIGN(offset, 16);
    }
}

static void emit_data(Obj* prog)
{
    println("  .data");
    for (Obj* var = prog; var; var = var->next) {
        if (var->isFunc || !var->isDefinition) {
            continue;
        }

        assert(!var->isLocal);

        println("# %s", var->name);
        println("  .align %d", var->type->align);
        if (!var->isStatic) {
            println("  .globl %s", var->name);
        }
        println("%s:", var->name);

        if (var->initData) {
            for (int i = 0; i < var->type->size; i++) {
                int c = 0xFF & (var->initData[i]);
                println("  .byte %d", c);
            }
        } else {
            println("  .zero %d", var->type->size);
        }
    }
}

static void store_gp(int r, int offset, int sz)
{
    switch (sz) {
    case 1:
        println("  mov %s, %d(%%rbp)", s_argreg8[r], offset);
        return;
    case 2:
        println("  mov %s, %d(%%rbp)", s_argreg16[r], offset);
        return;
    case 4:
        println("  mov %s, %d(%%rbp)", s_argreg32[r], offset);
        return;
    case 8:
        println("  mov %s, %d(%%rbp)", s_argreg64[r], offset);
        return;
    }

    assert(0);
}

static void emit_text(Obj* prog)
{
    println("  .text");
    for (Obj* fn = prog; fn; fn = fn->next) {
        if (!fn->isFunc || !fn->isDefinition) {
            continue;
        }

        if (!fn->isStatic) {
            println("  .global %s", fn->name);
        }
        println("%s:", fn->name);
        s_current_fn = fn;

        // Prologue
        println("  push %%rbp");
        println("  mov %%rsp, %%rbp");
        println("  sub $%d, %%rsp", fn->stackSize);
        // Emit code

        // Save arg registers if function is variadic
        if (fn->vaArea) {
            int gp = 0;
            for (Obj* var = fn->params; var; var = var->next) {
                gp++;
            }
            int off = fn->vaArea->offset;
            // va_elem
            println("  movl $%d, %d(%%rbp)", gp * 8, off);
            println("  movl $0, %d(%%rbp)", off + 4);
            println("  movq %%rbp, %d(%%rbp)", off + 16);
            println("  addq $%d, %d(%%rbp)", off + 24, off + 16);
            // __reg_save_area__
            println("  movq %%rcx, %d(%%rbp)", off + 24);
            println("  movq %%rdx, %d(%%rbp)", off + 32);
            println("  movq %%r8, %d(%%rbp)", off + 40);
            println("  movq %%r9, %d(%%rbp)", off + 48);
            println("  movsd %%xmm0, %d(%%rbp)", off + 56);
            println("  movsd %%xmm1, %d(%%rbp)", off + 64);
            println("  movsd %%xmm2, %d(%%rbp)", off + 72);
            println("  movsd %%xmm3, %d(%%rbp)", off + 80);
            println("  movsd %%xmm4, %d(%%rbp)", off + 88);
            println("  movsd %%xmm5, %d(%%rbp)", off + 96);
            println("  movsd %%xmm6, %d(%%rbp)", off + 104);
            println("  movsd %%xmm7, %d(%%rbp)", off + 112);
        }

        // Save passed-by-register arguments to the stack
        int i = 0;
        for (Obj* var = fn->params; var; var = var->next) {
            store_gp(i++, var->offset, var->type->size);
        }

        gen_stmt(fn->body);
        assert(s_depth == 0);
        // Epilogue
        println(".L.return.%s:", fn->name);
        println("  mov %%rbp, %%rsp");
        println("  pop %%rbp");
        println("  ret");
    }
}

void gen(Obj* prog, char* srcname, char* asmname)
{
    // @TODO: remove
    dummysetup();

    s_output = fopen(asmname, "w");

    assign_lvar_offsets(prog);

    println("  .file 1 \"%s\"", srcname);
    emit_text(prog);
    emit_data(prog);

    fclose(s_output);
    s_output = NULL;
}
