#ifndef DEFINE_NODE
#define DEFINE_NODE(...)
#endif

// clang-format off
//          enum            , binary-op     , unary-op
DEFINE_NODE(ND_INVALID      , 0             , 0         )
DEFINE_NODE(ND_ADD          , "+"           , 0         ) // +
DEFINE_NODE(ND_SUB          , "-"           , 0         ) // -
DEFINE_NODE(ND_MUL          , "*"           , 0         ) // *
DEFINE_NODE(ND_DIV          , "/"           , 0         ) // /
DEFINE_NODE(ND_NEG          , 0             , "-"       ) // unary -
DEFINE_NODE(ND_ADDR         , 0             , "&"       ) // unary &
DEFINE_NODE(ND_DEREF        , 0             , "*"       ) // unary *
DEFINE_NODE(ND_EQ           , "=="          , 0         ) // ==
DEFINE_NODE(ND_NE           , "!="          , 0         ) // !=
DEFINE_NODE(ND_LT           , "<"           , 0         ) // <
DEFINE_NODE(ND_LE           , "<="          , 0         ) // <=
DEFINE_NODE(ND_GT           , ">"           , 0         ) // >
DEFINE_NODE(ND_GE           , ">="          , 0         ) // >=
DEFINE_NODE(ND_ASSIGN       , 0             , 0         ) // =
DEFINE_NODE(ND_COMMA        , 0             , 0         ) // ,
DEFINE_NODE(ND_MEMBER       , 0             , 0         ) // . (struct member access)
DEFINE_NODE(ND_IF           , 0             , 0         ) // "if"
DEFINE_NODE(ND_FOR          , 0             , 0         ) // "for" or "while"
DEFINE_NODE(ND_RETURN       , 0             , 0         ) // "return"
DEFINE_NODE(ND_BLOCK        , 0             , 0         ) // { ... }
DEFINE_NODE(ND_FUNCCALL     , 0             , 0         ) // function call
DEFINE_NODE(ND_EXPR_STMT    , 0             , 0         ) // expression statement
DEFINE_NODE(ND_VAR          , 0             , 0         ) // variable
DEFINE_NODE(ND_NUM          , 0             , 0         ) // number literal
// clang-format on