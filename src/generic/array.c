#include "array.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static void array_new_cap(struct Array* arr, int newCap)
{
    assert(newCap > arr->capacity);
    assert(arr->eleSize);

    void* oldBuffer = arr->buffer;
    void* newBuffer = calloc(1, newCap * arr->eleSize);

    if (oldBuffer) {
        memcpy(newBuffer, oldBuffer, arr->eleSize * arr->capacity);
    }

    arr->buffer = newBuffer;
    arr->capacity = newCap;

    free(oldBuffer);
}

void array_init(struct Array* arr, int eleSize, int cap)
{
    arr->buffer = NULL;
    arr->len = arr->capacity = 0;
    arr->eleSize = eleSize;

    array_new_cap(arr, cap);
}

void array_clear(struct Array* arr)
{
    free(arr->buffer);
    arr->buffer = NULL;
    arr->eleSize = arr->len = arr->capacity = 0;
}

void* _array_at(struct Array* arr, int idx) {
    assert(arr->eleSize);
    assert(arr->buffer);
    assert(arr->capacity);

    if (idx >= arr->capacity) {
        // @TODO: find the best size to grow to
        assert(0);
        array_new_cap(arr, idx);
    }

    return ((char*)arr->buffer)[idx * arr->eleSize];
}
