#include "../src/generic/common.h"
#include "../src/utility.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

int util_test()
{
    printf("running util test...\n");
    // ALIGN()
    assert(16 == ALIGN(1, 16));
    assert(16 == ALIGN(8, 16));
    assert(16 == ALIGN(15, 16));
    assert(16 == ALIGN(16, 16));
    assert(32 == ALIGN(18, 16));
    assert(32 == ALIGN(31, 16));
    assert(32 == ALIGN(32, 16));
    // MIN() MAX()
    assert(MIN(-1, 4) == -1);
    assert(MIN(6, 4) == 4);
    assert(MAX(-1, 4) == 4);
    assert(MAX(6, 4) == 6);
    assert(MAX(-4, -8) == -4);

    // ARRAY_COUNTER()
    {
        int arr[4];
        STATIC_ASSERT(ARRAY_COUNTER(arr) == 4);
    }
    {
        char arr[8];
        STATIC_ASSERT(ARRAY_COUNTER(arr) == 8);
    }

    assert(streq("ABC", "ABC"));
    assert(!streq("AB", "ABC"));

    assert(startswithcase("abcdef", "AbC"));
    assert(!startswithcase("ab-def", "Abc"));
    assert(MAX_OSPATH == 512);

    {
        char buf[512];
        path_simplify("file.c", buf);
        assert(streq(buf, "file.c"));
    }
    {
        char buf[512];
        path_simplify("./file.c", buf);
        assert(streq(buf, "file.c"));
    }
    {
        char buf[512];
        path_simplify("../abc/file.c", buf);
        assert(streq(buf, "../abc/file.c"));
    }
    {
        char buf[512];
        path_simplify("abc/../file.c", buf);
        assert(streq(buf, "file.c"));
    }
    {
        char buf[512];
        path_simplify("abc/..//def/./file.c", buf);
        assert(streq(buf, "def/file.c"));
    }

    printf("util test passed\n");
    return 0;
}

#ifndef NO_MAIN
int main()
{
    return util_test();
}
#endif
