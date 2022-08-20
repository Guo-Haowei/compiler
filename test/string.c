#include "test.h"

int main()
{
    /* char */
    {
        char x = 1, y = 2, z[14];
        ASSERT(1, x);
        ASSERT(2, y);
        ASSERT(1, sizeof(x));
        ASSERT(14, sizeof(z));
    }

    ASSERT(0, ""[0]);
    ASSERT(1, sizeof(""));
    ASSERT(97, "abc"[0]);
    ASSERT(98, "abc"[1]);
    ASSERT(99, "abc"[2]);
    ASSERT(0, "abc"[3]);
    ASSERT(4, sizeof("abc"));
    ASSERT(1, sizeof("abc"[100]));

    /* escape */
    ASSERT(7, "\a8\b\t"[0]);
    ASSERT(48, "\a0\b\t"[1]);
    ASSERT(8, "\a0\b\t"[2]);
    ASSERT(9, "\a0\b\t"[3]);

    ASSERT(10, "\n"[0]);
    ASSERT(11, "\v"[0]);
    ASSERT(12, "\f"[0]);
    ASSERT(13, "\r"[0]);
    ASSERT(27, "\e"[0]);
    ASSERT(106, "\j"[0]);
    ASSERT(107, "\k"[0]);
    ASSERT(108, "\l"[0]);
    ASSERT(7, "\ax\ny"[0]);
    ASSERT(120, "\ax\ny"[1]);
    ASSERT(10, "\ax\ny"[2]);
    ASSERT(121, "\ax\ny"[3]);
    ASSERT(5, sizeof("\ax\ny"));

    ASSERT(0, "\0"[0]);
    ASSERT(16, "\20"[0]);
    ASSERT(65, "\101"[0]);
    ASSERT(104, "\1500"[0]);

    ASSERT(0, "\x00"[0]);
    ASSERT(119, "\x77"[0]);
    ASSERT(165 - 256, "\xA5"[0]);
    ASSERT(255 - 256, "\x00ff"[0]);

    printf("OK\n");
    return 0;
}