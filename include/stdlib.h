#ifndef __STDLIB_H__
#define __STDLIB_H__

#define NULL ((void*)0)

void* calloc(int numElements, int sizeOfElements);
void free(void* p);
void exit(int code);

#endif