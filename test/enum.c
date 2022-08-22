#include "test.h"

int main()
{
    // clang-format off
    {
        enum { zero, one, two, five = 5, six, seven, three = 3, four };
        ASSERT(0, zero);
        ASSERT(1, one);
        ASSERT(2, two);
        ASSERT(3, three);
        ASSERT(4, four);
        ASSERT(5, five);
        ASSERT(6, six);
        ASSERT(7, seven);
    }
    {
        enum Fruit {
            Apple,
            Banana,
            Grape,
        };
        ASSERT(4, sizeof(enum Fruit));
        ASSERT(4, sizeof(Apple));
        typedef enum Fruit Fruit;
        Fruit t = Apple + Banana;
        ASSERT(1, t);
    }

    printf("OK\n");
    return 0;
}
