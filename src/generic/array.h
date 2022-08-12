#ifndef __ARRAY_H__
#define __ARRAY_H__

struct Array {
    void* buffer;
    int len;
    int capacity;
    int eleSize;
};

void array_init(struct Array* arr, int eleSize, int cap);
void array_clear(struct Array* arr);

#define array_at(T, arr, idx) ((T*)_array_at(arr, idx))
void* _array_at(struct Array* arr, int idx);

#endif // ifndef __ARRAY_H__
