#include <stdio.h>

int assert_impl(int a, int b, char* msg, int ln);
#define ASSERT(_A_, _B_) assert_impl(_A_, _B_, #_B_, __LINE__)
