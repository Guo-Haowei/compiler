#define assert(a, b) assert_impl(a, b, #b, __LINE__)

int printf(char* fmt);
int assert_implt(int a, int b, char* msg, int ln);
