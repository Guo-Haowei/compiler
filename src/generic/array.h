#ifndef __ARRAY_H__
#define __ARRAY_H__
#include <assert.h>

typedef struct {
    char* buffer;
    int len;
    int capacity;
    int eleSize;
} Array;

Array* array_new(int eleSize, int cap);
void array_init(Array* arr, int eleSize, int cap);
void array_clear(Array* arr);

#define array_at(T, arr, idx) ((T*)_array_at((arr), (idx)))
void* _array_at(Array* arr, int idx);

void* array_back(Array* arr);

#define array_push_back(T, arr, ele) _array_push_back(arr, &ele)

void _array_push_back(Array* arr, void* data);

#endif // ifndef __ARRAY_H__
