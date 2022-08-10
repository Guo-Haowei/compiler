#include "test.h"

int main()
{
    /* char */
    {
        char x = 1, y = 2, z[14];
        assert(1, x);
        assert(2, y);
        assert(1, sizeof(x));
        assert(14, sizeof(z));
    }

    assert(0, ""[0]);
    assert(1, sizeof(""));
    assert(97, "abc"[0]);
    assert(98, "abc"[1]);
    assert(99, "abc"[2]);
    assert(0, "abc"[3]);
    assert(4, sizeof("abc"));
    assert(1, sizeof("abc"[100]));

    /* escape */
    assert(7, "\a8\b\t"[0]);
    assert(48, "\a0\b\t"[1]);
    assert(8, "\a0\b\t"[2]);
    assert(9, "\a0\b\t"[3]);

    assert(10, "\n"[0]);
    assert(11, "\v"[0]);
    assert(12, "\f"[0]);
    assert(13, "\r"[0]);
    assert(27, "\e"[0]);
    assert(106, "\j"[0]);
    assert(107, "\k"[0]);
    assert(108, "\l"[0]);
    assert(7, "\ax\ny"[0]);
    assert(120, "\ax\ny"[1]);
    assert(10, "\ax\ny"[2]);
    assert(121, "\ax\ny"[3]);
    assert(5, sizeof("\ax\ny"));

    assert(0, "\0"[0]);
    assert(16, "\20"[0]);
    assert(65, "\101"[0]);
    assert(104, "\1500"[0]);

    assert(0, "\x00"[0]);
    assert(119, "\x77"[0]);
    assert(165 - 256, "\xA5"[0]);
    assert(255 - 256, "\x00ff"[0]);

    printf("OK\n");
    return 0;
}