#include "../src/minic.h"
#include <string.h>

int main()
{
    {
        char* actual = format("hello %% %s, %d, %.*s\n", "ab", 1, 4, "123456789");
        char* expect = "hello % ab, 1, 1234\n";
        assert(strcmp(actual, expect) == 0);
    }

    return 0;
}
