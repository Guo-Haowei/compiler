#include "vector.h"

#include "common.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

static void vector_new_cap(Vector* vec, int newCap)
{
    assert(newCap > vec->capacity);
    assert(vec->eleSize);

    void* oldBuffer = vec->buffer;
    void* newBuffer = calloc(1, newCap * vec->eleSize);

    if (oldBuffer) {
        memcpy(newBuffer, oldBuffer, vec->eleSize * vec->capacity);
    }

    vec->buffer = newBuffer;
    vec->capacity = newCap;

    free(oldBuffer);
}

void vector_init(Vector* vec, int eleSize, int cap)
{
    vec->buffer = NULL;
    vec->len = vec->capacity = 0;
    vec->eleSize = eleSize;

    vector_new_cap(vec, cap);
}

Vector* vector_new(int eleSize, int cap)
{
    Vector* vec = calloc(1, sizeof(Vector));
    vector_init(vec, eleSize, cap);
    return vec;
}

void vector_clear(Vector* vec)
{
    free(vec->buffer);
    vec->buffer = NULL;
    vec->eleSize = vec->len = vec->capacity = 0;
}

void* _vector_at(Vector* vec, int idx)
{
    assert(vec->len > idx);

    return vec->buffer + idx * vec->eleSize;
}

void* vector_back(Vector* vec)
{
    return _vector_at(vec, vec->len - 1);
}

void _vector_push_back(Vector* vec, void* data)
{
    assert(vec->eleSize);
    assert(vec->buffer);
    assert(vec->capacity);
    assert(vec->len <= vec->capacity);

    if (vec->len + 1 > vec->capacity) {
        int newCap = vec->capacity * 2;
        vector_new_cap(vec, newCap);
    }

    char* ptr = vec->buffer + vec->eleSize * vec->len;
    memcpy(ptr, data, vec->eleSize);
    vec->len = vec->len + 1;
}
