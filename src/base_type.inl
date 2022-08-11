#ifndef DEFINE_BASE_TYPE
#define DEFINE_BASE_TYPE(...)
#endif

// clang-format off
//               name   , enum      , size  , align
DEFINE_BASE_TYPE(void   , TY_VOID   , 1     , 1    )
DEFINE_BASE_TYPE(char   , TY_CHAR   , 1     , 1    )
DEFINE_BASE_TYPE(short  , TY_SHORT  , 2     , 2    )
DEFINE_BASE_TYPE(int    , TY_INT    , 4     , 4    )
DEFINE_BASE_TYPE(long   , TY_LONG   , 8     , 8    )
// clang-format on
