#ifndef __STDLIB_H__
#define __STDLIB_H__
#include <stdint.h>

#define NULL ((void*)0)

void* malloc(int size);
void* calloc(int numElements, int sizeOfElements);
void free(void* p);
void exit(int code);

uint64_t strtoull(char* str, char** endptr, int base);

#endif