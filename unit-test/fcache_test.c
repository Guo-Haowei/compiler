#include "../src/minic.h"

#include <assert.h>

#define assert_eq(str1, str2) assert(strcmp(str1, str2) == 0)

int fcache_test()
{
    printf("running fcache test...\n");

    #define BASE_PATH "D:\\workspace"
    {
        char buf[512];
        fcache_resolve_path(BASE_PATH, "file.c", buf);
        assert_eq(buf, "D:/workspace/file.c");
    }
    {
        char buf[512];
        fcache_resolve_path(BASE_PATH, "./file.c", buf);
        assert_eq(buf, "D:/workspace/file.c");
    }
    {
        char buf[512];
        fcache_resolve_path(BASE_PATH, "../file.c", buf);
        assert_eq(buf, "D:/file.c");
    }
    {
        char buf[512];
        fcache_resolve_path(BASE_PATH, "../src/file.c", buf);
        assert_eq(buf, "D:/src/file.c");
    }
    {
        char buf[512];
        fcache_resolve_path(BASE_PATH, "../a/../bbbb/./src/file.c", buf);
        assert_eq(buf, "D:/bbbb/src/file.c");
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
