#ifndef DEFINE_NODE
#define DEFINE_NODE(...)
#endif

// clang-format off
//          enum            , debug string
DEFINE_NODE(ND_INVALID      , "<error>" )
DEFINE_NODE(ND_ADD          , "+"       )
DEFINE_NODE(ND_SUB          , "-"       )
DEFINE_NODE(ND_MUL          , "*"       )
DEFINE_NODE(ND_DIV          , "/"       )
DEFINE_NODE(ND_REM          , "%"       )
DEFINE_NODE(ND_NEG          , "neg"     )

DEFINE_NODE(ND_EQ           , "=="      )
DEFINE_NODE(ND_NE           , "!="      )
DEFINE_NODE(ND_LT           , "<"       )
DEFINE_NODE(ND_LE           , "<="      )
DEFINE_NODE(ND_GT           , ">"       )
DEFINE_NODE(ND_GE           , ">="      )

DEFINE_NODE(ND_EXPR_STMT    , "<expr>"  )
DEFINE_NODE(ND_NUM          , "<num>"   )
// clang-format on