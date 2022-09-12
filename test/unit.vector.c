#include "../src/generic/vector.h"

#include <stdio.h>
#include <stdlib.h>

static void print_vector(Vector* vec)
{
    printf("[");
    for (int i = 0; i < vec->len; ++i) {
        int a = *vector_at(int, vec, i);
        printf("%d,", a);
    }
    printf("]\n");
}

int vector_test()
{
    printf("running vector test...\n");
    Vector _vec;
    Vector* vec = &_vec;

    int target = 16;
    vector_init(vec, sizeof(int), 8);

    for (int i = 0; i < target; ++i) {
        int a = target - i;
        vector_push_back(int, vec, a);
    }

    assert(vec->capacity == target);
    assert(vec->len == target);
    assert(vec->eleSize == sizeof(int));
    for (int i = 0; i < target; ++i) {
        assert(target - i == *vector_at(int, vec, i));
    }

    print_vector(vec);
    vector_clear(vec);
    printf("vector test passed\n");
    return 0;
}

#ifndef NO_MAIN
int main()
{
    return vector_test();
}
#endif
