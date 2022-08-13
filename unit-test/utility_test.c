#include "../src/minic.h"

#include <assert.h>

#define assert_eq(str1, str2) assert(strcmp(str1, str2) == 0)

int fcache_test()
{
    printf("running fcache test...\n");

    {
        char buf[512];
        path_simplify("file.c", buf);
        assert_eq(buf, "file.c");
    }
    {
        char buf[512];
        path_simplify("./file.c", buf);
        assert_eq(buf, "file.c");
    }
    {
        char buf[512];
        path_simplify("../abc/file.c", buf);
        assert_eq(buf, "../abc/file.c");
    }
    {
        char buf[512];
        path_simplify("abc/../file.c", buf);
        assert_eq(buf, "file.c");
    }
    {
        char buf[512];
        path_simplify("abc/..//def/./file.c", buf);
        assert_eq(buf, "def/file.c");
    }

    printf("fcache test passed\n\n");
    return 0;
}

#ifdef TEST_STANDALONE
int main()
{
    return fcache_test();
}
#endif
