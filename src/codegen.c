#include "minic.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

// clang-format off
enum { I8, I16, I32, I64 };
// clang-format on

// @TODO: refactor
static int s_depth = 0;
static char* s_argreg8[] = { "%cl", "%dl", "%r8b", "%r9b" };
static char* s_argreg16[] = { "%cx", "%dx", "%r8w", "%r9w" };
static char* s_argreg32[] = { "%ecx", "%edx", "%r8d", "%r9d" };
static char* s_argreg64[] = { "%rcx", "%rdx", "%r8", "%r9" };
static Obj* s_current_fn;

static FILE* s_output;

static void writeln(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(s_output, fmt, ap);
    fprintf(s_output, "\n");
    va_end(ap);
}

static void push()
{
    writeln("  push %%rax");
    s_depth++;
}

static void pop(char const* arg)
{
    writeln("  pop %s", arg);
    s_depth--;
}

static void gen_expr(Node const* node);

// Load a value from where %rax is pointing to.
static void load(Type* type)
{
    if (type->eTypeKind == TY_ARRAY) {
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
        writeln("  movsbl (%%rax), %%eax");
        break;
    case 2:
        writeln("  movswl (%%rax), %%eax");
        break;
    case 4:
        writeln("  movsxd (%%rax), %%rax");
        break;
    case 8:
        writeln("  mov (%%rax), %%rax");
        break;
    default:
        UNREACHABLE();
        break;
    }
}

// Store %rax to an address that the stack top is pointing to.
static void store(Type* type)
{
    pop("%rdi");
    switch (type->size) {
    case 1:
        writeln("  mov %%al, (%%rdi)");
        break;
    case 2:
        writeln("  mov %%ax, (%%rdi)");
        break;
    case 4:
        writeln("  mov %%eax, (%%rdi)");
        break;
    case 8:
        writeln("  mov %%rax, (%%rdi)");
        break;
    default:
        UNREACHABLE();
        break;
    }
}

// Compute the absolute address of a given node.
// It's an error if a given node does not reside in memory.
static void gen_addr(Node const* node)
{
    switch (node->eNodeKind) {
    case ND_VAR:
        if (node->var->isLocal) {
            // local
            writeln("  lea %d(%%rbp), %%rax", node->var->offset);
        } else {
            // global
            writeln("  lea %s(%%rip), %%rax", node->var->name);
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
        writeln("  add $%d, %%rax", node->member->offset);
        return;
    default:
        // @TODO: remove break
        break;
    }

    error_tok(node->tok, "not an lvalue");
}

static void gen_cmp_expr(NodeKind eNodeKind, const char* di, const char* ax)
{
    static char const* const s_cmds[] = {
        "sete",
        "setne",
        "setl",
        "setle",
        "setg",
        "setge",
    };

    STATIC_ASSERT(ARRAY_COUNTER(s_cmds) == (ND_GE - ND_EQ + 1));

    int const index = eNodeKind - ND_EQ;
    ASSERT_IDX(index, ARRAY_COUNTER(s_cmds));

    writeln("  cmp %s, %s", di, ax);
    writeln("  %s %%al", s_cmds[index]);
    writeln("  movzb %%al, %%rax");
}

static int getTypeId(Type* ty)
{
    switch (ty->eTypeKind) {
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

// The table for type casts
static char s_i32i8[] = "movsbl %al, %eax";
static char s_i32i16[] = "movswl %ax, %eax";
static char s_i32i64[] = "movsxd %eax, %rax";

static char* s_cast_table[][10] = {
    { NULL, NULL, NULL, s_i32i64 },        // i8
    { s_i32i8, NULL, NULL, s_i32i64 },     // i16
    { s_i32i8, s_i32i16, NULL, s_i32i64 }, // i32
    { s_i32i8, s_i32i16, NULL, NULL },     // i64
};

static void gen_cast(Type* from, Type* to)
{
    if (to->eTypeKind == TY_VOID) {
        return;
    }

    int t1 = getTypeId(from);
    int t2 = getTypeId(to);
    if (s_cast_table[t1][t2]) {
        writeln("  %s", s_cast_table[t1][t2]);
    }
}

static void gen_expr(Node const* node)
{
    switch (node->eNodeKind) {
    case ND_NUM:
        writeln("  mov $%ld, %%rax", node->val);
        return;
    case ND_NEG:
        gen_expr(node->lhs);
        // if (node->type->size == 8) {
        writeln("  neg %%rax");
        // } else {
        //     writeln("  neg %%eax");
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
    case ND_FUNCCALL:
        Node* arg = node->args;
        for (int i = 0; i < node->argc; ++i, arg = arg->next) {
            gen_expr(arg);
            push();
        }
        assert(arg == NULL);

        for (int i = node->argc - 1; i >= 0; --i) {
            pop(s_argreg64[i]);
        }

        writeln("  mov $0, %%rax");
        writeln("  call %s", node->funcname);
        return;
    case ND_CAST:
        gen_expr(node->lhs);
        gen_cast(node->lhs->type, node->type);
        return;
    default:
        break;
    }

    gen_expr(node->rhs);
    push();
    gen_expr(node->lhs);
    pop("%rdi");

    const char *ax = NULL, *di = NULL;
    if (node->lhs->type->eTypeKind == TY_LONG || node->lhs->type->base) {
        ax = "%rax";
        di = "%rdi";
    } else {
        ax = "%eax";
        di = "%edi";
    }

    switch (node->eNodeKind) {
    case ND_ADD:
        writeln("  add %s, %s", di, ax);
        return;
    case ND_SUB:
        writeln("  sub %s, %s", di, ax);
        return;
    case ND_MUL:
        writeln("  imul %s, %s", di, ax);
        return;
    case ND_DIV:
        if (node->lhs->type->size == 8) {
            writeln("  cqo");
        } else {
            writeln("  cdq");
        }
        writeln("  idiv %s", di);
        return;
    case ND_EQ:
    case ND_NE:
    case ND_LT:
    case ND_LE:
    case ND_GT:
    case ND_GE:
        gen_cmp_expr(node->eNodeKind, di, ax);
        return;
    default:
        break;
    }

    error_tok(node->tok, "invalid statement");
}

// TODO: refactor
static int label_counter()
{
    static int i = 1;
    return i++;
}

static void gen_stmt(Node const* node)
{
    writeln("  .loc 1 %d", node->tok->line);

    switch (node->eNodeKind) {
    case ND_IF: {
        const int c = label_counter();
        gen_expr(node->cond);
        writeln("  cmp $0, %%rax");
        writeln("  je  .L.else.%d", c);
        gen_stmt(node->then);
        writeln("  jmp .L.end.%d", c);
        writeln(".L.else.%d:", c);
        if (node->els) {
            gen_stmt(node->els);
        }
        writeln(".L.end.%d:", c);
        return;
    }
    case ND_EXPR_STMT:
        gen_expr(node->lhs);
        return;
    case ND_RETURN:
        gen_expr(node->lhs);
        writeln("  jmp .L.return.%s", s_current_fn->name);
        return;
    case ND_BLOCK:
        for (Node* n = node->body; n; n = n->next) {
            gen_stmt(n);
        }
        return;
    case ND_FOR: {
        int const c = label_counter();
        if (node->init) {
            gen_stmt(node->init);
        }

        writeln(".L.begin.%d:", c);

        if (node->cond) {
            gen_expr(node->cond);
            writeln("  cmp $0, %%rax");
            writeln("  je  .L.end.%d", c);
        }

        gen_stmt(node->then);

        if (node->inc) {
            gen_expr(node->inc);
        }
        writeln("  jmp .L.begin.%d", c);
        writeln(".L.end.%d:", c);
        return;
    }
    default:
        break;
    }

    error_tok(node->tok, "invalid statement");
}

static List* build_reversed_var_list(Obj* var)
{
    List* list = list_new();
    for (Obj* c = var; c; c = c->next) {
        size_t ptr = (size_t)c;
        list_push_front(list, ptr);
    }
    return list;
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
            offset = ALIGN_TO(offset, var->type->align);
            var->offset = -offset;
        }
        fn->stackSize = ALIGN_TO(offset, 16);
    }
}

static void emit_data(Obj* prog)
{
    writeln("  .data");
    for (Obj* var = prog; var; var = var->next) {
        if (var->isFunc) {
            continue;
        }

        assert(!var->isLocal);

        writeln("  .globl %s", var->name);
        Token* tok = var->tok;
        if (tok) {
            writeln("# %s", tok->raw);
        }
        writeln("%s:", var->name);

        if (var->initData) {
            for (int i = 0; i < var->type->size; i++) {
                uint32_t c = (unsigned char)(var->initData[i]);
                writeln("  .byte %u", c);
            }
        } else {
            writeln("  .zero %d", var->type->size);
        }
    }
}

static void store_gp(int r, int offset, int sz)
{
    switch (sz) {
    case 1:
        writeln("  mov %s, %d(%%rbp)", s_argreg8[r], offset);
        return;
    case 2:
        writeln("  mov %s, %d(%%rbp)", s_argreg16[r], offset);
        return;
    case 4:
        writeln("  mov %s, %d(%%rbp)", s_argreg32[r], offset);
        return;
    case 8:
        writeln("  mov %s, %d(%%rbp)", s_argreg64[r], offset);
        return;
    }
    UNREACHABLE();
}

static void emit_text(Obj* prog)
{
    writeln("  .text");
    for (Obj* fn = prog; fn; fn = fn->next) {
        if (!fn->isFunc || !fn->isDefinition) {
            continue;
        }

        writeln("  .globl %s", fn->name);
        writeln("%s:", fn->name);
        s_current_fn = fn;

        // Prologue
        writeln("  push %%rbp");
        writeln("  mov %%rsp, %%rbp");
        writeln("  sub $32, %%rsp"); // HACK: fix main segfault

        // print local variables
        List* locals = build_reversed_var_list(fn->locals);
        for (ListNode* c = locals->front; c; c = c->next) {
            Obj* var = *(Obj**)(c + 1);
            writeln("  # var %s [%d]", var->name, var->offset);
        }
        list_delete(locals);

        writeln("  sub $%d, %%rsp", fn->stackSize);
        // Emit code

        // Save passed-by-register arguments to the stack
        int i = 0;
        for (Obj* var = fn->params; var; var = var->next) {
            store_gp(i++, var->offset, var->type->size);
        }

        gen_stmt(fn->body);
        assert(s_depth == 0);
        // Epilogue
        writeln(".L.return.%s:", fn->name);
        writeln("  mov %%rbp, %%rsp");
        writeln("  pop %%rbp");
        writeln("  ret");
    }
}

void gen(Obj* prog, const char* srcname, const char* asmname)
{
    s_output = fopen(asmname, "w");

    assign_lvar_offsets(prog);

    writeln("  .file 1 \"%s\"", srcname);
    emit_text(prog);
    emit_data(prog);

    fclose(s_output);
}
