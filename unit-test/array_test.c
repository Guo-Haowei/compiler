#include "../src/generic/array.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

static void print_array(struct Array* arr) {
    printf("array(size:%d, cap:%d) [ ", arr->len, arr->capacity);

    for (int i = 0; i < arr->len; ++i) {
        printf("%d ", ((int*)arr->buffer)[i]);
    }
    printf("]\n");
}

int array_test()
{
    printf("running array test...\n");
    struct Array _arr;
    struct Array* arr = &_arr;

    int target = 16;
    array_init(arr, sizeof(int), 8);

    for (int i = 0; i < target; ++i) {
        int a = target - i;
        array_push_back(int, arr, a);
    }

    assert(arr->capacity == target);
    assert(arr->len == target);
    assert(arr->eleSize == sizeof(int));

    print_array(arr);

    array_clear(arr);
    printf("array test passed\n\n");
    return 0;
}

#ifdef TEST_STANDALONE 
int main() {
    return array_test();
}
#endif
