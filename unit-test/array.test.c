#include "../src/generic/array.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(_MSC_VER)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

int main()
{
    struct Array _arr;
    struct Array* arr = &_arr;

    const int target = 16;
    array_init(arr, sizeof(int), 8);

    for (int i = 0; i < target; ++i) {
        int a = i + 1;
        array_push_back(int, arr, a);
    }

    assert(arr->capacity == target);
    assert(arr->len == target);
    assert(arr->eleSize == sizeof(int));

    printf("[ ");
    for (int i = 0; i < arr->capacity; ++i) {
        printf("%d ", *(int*)array_at(int, arr, i));
    }
    printf("]\n");

#if defined(_MSC_VER)
    _CrtDumpMemoryLeaks();
#endif
}

