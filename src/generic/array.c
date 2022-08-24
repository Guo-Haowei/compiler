#include "array.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static void array_new_cap(Array* arr, int newCap)
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

void array_init(Array* arr, int eleSize, int cap)
{
    arr->buffer = NULL;
    arr->len = arr->capacity = 0;
    arr->eleSize = eleSize;

    array_new_cap(arr, cap);
}

Array* array_new(int eleSize, int cap)
{
    Array* arr = calloc(1, sizeof(Array));
    array_init(arr, eleSize, cap);
    return arr;
}

void array_clear(Array* arr)
{
    free(arr->buffer);
    arr->buffer = NULL;
    arr->eleSize = arr->len = arr->capacity = 0;
}

void* _array_at(Array* arr, int idx)
{
    assert(arr->len > idx);

    return arr->buffer + idx * arr->eleSize;
}

void* array_back(Array* arr)
{
    return _array_at(arr, arr->len - 1);
}

void _array_push_back(Array* arr, void* data)
{
    assert(arr->eleSize);
    assert(arr->buffer);
    assert(arr->capacity);
    assert(arr->len <= arr->capacity);

    if (arr->len + 1 > arr->capacity) {
        int newCap = arr->capacity * 2;
        array_new_cap(arr, newCap);
    }

    char* ptr = arr->buffer + arr->eleSize * arr->len;
    memcpy(ptr, data, arr->eleSize);
    arr->len = arr->len + 1;
}
