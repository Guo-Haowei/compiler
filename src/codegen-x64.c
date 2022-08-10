#include "minic.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

// @TODO: refactor
static int s_depth = 0;
static char* s_argreg8[] = { "%cl", "%dl", "%r8b", "%r9b" };
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

    switch (type->size) {
    case 1:
        writeln("  movsbq (%%rax), %%rax");
        break;
    case 8:
        writeln("  mov (%%rax), %%rax");
        break;
    default:
        unreachable();
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
    case 8:
        writeln("  mov %%rax, (%%rdi)");
        break;
    default:
        unreachable();
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
            writeln("  lea %d(%%rbp), %%rax", node->var->offset);
        } else {
            writeln("  lea %s(%%rip), %%rax", node->var->name);
        }
        return;
    case ND_DEREF:
        gen_expr(node->lhs);
        return;
    default:
        break;
    }

    error_tok(node->tok, "not an lvalue");
}

static void gen_cmp_expr(NodeKind eNodeKind)
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
    assertindex(index, ARRAY_COUNTER(s_cmds));

    writeln("  cmp %%rdi, %%rax");
    writeln("  %s %%al", s_cmds[index]);
    writeln("  movzb %%al, %%rax");
}

static void gen_expr(Node const* node)
{
    writeln(".loc 1 %d", node->tok->line);

    switch (node->eNodeKind) {
    case ND_NUM:
        writeln("  mov $%d, %%rax", node->val);
        return;
    case ND_NEG:
        gen_expr(node->lhs);
        writeln("  neg %%rax");
        return;
    case ND_VAR:
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
    case ND_FUNCCALL:
        Node* arg = node->args;
        for (int i = 0; i < node->argc; ++i, arg = arg->next) {
            gen_expr(arg);
            push();
        }
        assert(arg == nullptr);

        for (int i = node->argc - 1; i >= 0; --i) {
            pop(s_argreg64[i]);
        }

        writeln("  mov $0, %%rax");
        writeln("  call %s", node->funcname);
        return;
    default:
        break;
    }

    gen_expr(node->rhs);
    push();
    gen_expr(node->lhs);
    pop("%rdi");

    switch (node->eNodeKind) {
    case ND_ADD:
        writeln("  add %%rdi, %%rax");
        return;
    case ND_SUB:
        writeln("  sub %%rdi, %%rax");
        return;
    case ND_MUL:
        writeln("  imul %%rdi, %%rax");
        return;
    case ND_DIV:
        writeln("  cqo");
        writeln("  idiv %%rdi");
        return;
    case ND_EQ:
    case ND_NE:
    case ND_LT:
    case ND_LE:
    case ND_GT:
    case ND_GE:
        gen_cmp_expr(node->eNodeKind);
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
            var->offset = -offset;
        }
        fn->stackSize = align_to(offset, 16);
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
            writeln("# %.*s", tok->len, tok->start);
        }
        writeln("%s:", var->name);

        if (var->initData) {
            for (int i = 0; i < var->type->size; i++) {
                uint c = (unsigned char)(var->initData[i]);
                writeln("  .byte %u", c);
            }
        } else {
            writeln("  .zero %d", var->type->size);
        }
    }
}

static void emit_text(Obj* prog)
{
    writeln("  .text");
    for (Obj* fn = prog; fn; fn = fn->next) {
        if (!fn->isFunc) {
            continue;
        }

        writeln("  .globl %s", fn->name);
        writeln("%s:", fn->name);
        s_current_fn = fn;
        // Prologue
        writeln("  push %%rbp");
        writeln("  mov %%rsp, %%rbp");
        writeln("  sub $32, %%rsp"); // HACK: fix main segfault
        for (Obj* var = fn->locals; var; var = var->next) {
            writeln("  # var %s [%d]", var->name, var->offset);
        }
        writeln("  sub $%d, %%rsp", fn->stackSize);
        // Emit code

        // Save passed-by-register arguments to the stack
        int i = 0;
        for (Obj* var = fn->params; var; var = var->next) {
            if (var->type->size == 1) {
                writeln("  mov %s, %d(%%rbp)", s_argreg8[i++], var->offset);
            } else {
                writeln("  mov %s, %d(%%rbp)", s_argreg64[i++], var->offset);
            }
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

void gen(Obj* prog, const char* inputName)
{
    char outName[512];
    const size_t size = strlen(inputName);
    assert(size + 3 <= sizeof(outName)); // ".c\0"

    strncpy(outName, inputName, sizeof(outName));
    char* p = strrchr(outName, '.');
    if (!p) {
        p = outName + size;
    }
    p[1] = 's';
    p[2] = '\0';

    s_output = fopen(outName, "w");

    assign_lvar_offsets(prog);

    writeln("  .file 1 \"%s\"", inputName);
    emit_text(prog);
    emit_data(prog);

    fclose(s_output);
}
