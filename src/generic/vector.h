#ifndef __GENERIC_VECTOR_H__
#define __GENERIC_VECTOR_H__
#include <assert.h>

typedef struct {
    char* buffer;
    int len;
    int capacity;
    int eleSize;
} Vector;

Vector* vector_new(int eleSize, int cap);
void vector_init(Vector* vec, int eleSize, int cap);
void vector_clear(Vector* vec);

#define vector_at(T, vec, idx) ((T*)_vector_at((vec), (idx)))
void* _vector_at(Vector* vec, int idx);

void* vector_back(Vector* vec);

#define vector_push_back(T, vec, ele) _vector_push_back((vec), &ele)

void _vector_push_back(Vector* vec, void* data);

#endif
