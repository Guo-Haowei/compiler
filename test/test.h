int printf(char* fmt);
int assert(int a, int b);

// HACK: tmp
// #define assert(a, b) assert_impl(a, b, #b, __LINE__)
// int assert_impl(int a, int b, char* msg, int ln);
