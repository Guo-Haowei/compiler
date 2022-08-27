#ifndef __GENERIC_DICT_H__
#define __GENERIC_DICT_H__
#include <stdint.h>

#include "common.h"

typedef struct DictEntry DictEntry;
struct DictEntry {
    DictEntry* next;
    char* key;
    uint32_t hash;
    void* data;
};

typedef struct {
    int size;
    int bucketSize;
    DictEntry** bucket;
} Dict;

Dict* dict_new();
void dict_clear(Dict* dict);

void* dict_get(Dict* dict, char* key);
bool dict_has_key(Dict* dict, char* key);
bool dict_try_add(Dict* dict, char* key, void* data);
bool dict_erase(Dict* dict, char* key);

#endif
