#ifndef __ARRAY_H__
#define __ARRAY_H__
#include <assert.h>

struct Array {
    char* buffer;
    int len;
    int capacity;
    int eleSize;
};

struct Array* array_new(int eleSize, int cap);
void array_init(struct Array* arr, int eleSize, int cap);
void array_clear(struct Array* arr);

#define array_at(T, arr, idx) ((T*)_array_at((arr), (idx)))
void* _array_at(struct Array* arr, int idx);

void* array_back(struct Array* arr);

#define array_push_back(T, arr, ele)       \
    {                                      \
        assert(sizeof(T) == arr->eleSize); \
        _array_push_back(arr, &ele);       \
    }                                      \
    (void)0
void _array_push_back(struct Array* arr, const void* data);

#endif // ifndef __ARRAY_H__
