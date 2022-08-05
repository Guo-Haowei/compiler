#ifndef DEFINE_NODE
#define DEFINE_NODE(...)
#endif

DEFINE_NODE(ND_INVALID)
DEFINE_NODE(ND_ADD)       // +
DEFINE_NODE(ND_SUB)       // -
DEFINE_NODE(ND_MUL)       // *
DEFINE_NODE(ND_DIV)       // /
DEFINE_NODE(ND_REM)       // %
DEFINE_NODE(ND_NEG)       // -
DEFINE_NODE(ND_EQ)        // ==
DEFINE_NODE(ND_NE)        // !=
DEFINE_NODE(ND_LT)        // <
DEFINE_NODE(ND_LE)        // <=
DEFINE_NODE(ND_GT)        // >
DEFINE_NODE(ND_GE)        // >=
DEFINE_NODE(ND_ASSIGN)    // =
DEFINE_NODE(ND_IF)        // "if"
DEFINE_NODE(ND_FOR)       // "for"
DEFINE_NODE(ND_RETURN)    // "return"
DEFINE_NODE(ND_BLOCK)     // { ... }
DEFINE_NODE(ND_EXPR_STMT) // expression statement
DEFINE_NODE(ND_VAR)       // variable
DEFINE_NODE(ND_NUM)       // number literal