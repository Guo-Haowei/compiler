#include "dict.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define INIT_SIZE 15
#define HIGH_WATERMARK 70
#define TOMBSTONE ((void*)-1) // deleted entry

// https://stackoverflow.com/questions/7666509/hash-function-for-string
static uint32_t hash_str(char* str)
{
    uint32_t hash = 5381;
    for (; *str; ++str) {
        hash = ((hash << 5) + hash) + *str; // hash * 33 + c
    }
    return hash;
}

Dict* dict_new()
{
    Dict* dict = calloc(1, sizeof(Dict));
    dict->bucketSize = INIT_SIZE;
    dict->bucket = calloc(1, sizeof(DictEntry*) * dict->bucketSize);
    return dict;
}

void dict_clear(Dict* dict)
{
    assert(dict);
    // for (int i = 0; i < dict->bucketSize; ++i) {
    //     for (DictEntry* e = dict->bucket[i]; e;) {
    //         DictEntry* tmp = e;
    //         free(tmp);
    //         e = e->next;
    //     }
    // }
    if (dict->bucket) {
        free(dict->bucket);
    }
}

static DictEntry* dict_get_internal(Dict* dict, char* key)
{
    assert(dict);
    uint32_t hash = hash_str(key);
    int slot = hash % dict->bucketSize;

    for (DictEntry* n = dict->bucket[slot]; n; n = n->next) {
        if (strcmp(n->key, key) == 0) {
            return n;
        }
    }
    return NULL;
}

void* dict_get(Dict* dict, char* key)
{
    DictEntry* entry = dict_get_internal(dict, key);
    if (!entry) {
        return NULL;
    }
    return entry->data == TOMBSTONE ? NULL : entry->data;
}

bool dict_has_key(Dict* dict, char* key)
{
    DictEntry* entry = dict_get_internal(dict, key);
    if (!entry) {
        return false;
    }

    return entry->data != TOMBSTONE;
}

bool dict_try_add(Dict* dict, char* key, void* data)
{
    DictEntry* entry = dict_get_internal(dict, key);
    if (entry && entry->data != TOMBSTONE) {
        return false;
    }

    uint32_t hash = hash_str(key);

    if (!entry) {
        entry = calloc(1, sizeof(DictEntry));
    }
    entry->key = key;
    entry->data = data;
    entry->hash = hash;

    int slot = hash % dict->bucketSize;

    if (dict->bucket[slot]) {
        entry->next = dict->bucket[slot];
    }
    dict->bucket[slot] = entry;

    ++dict->size;

    if (dict->size * 100 > dict->bucketSize * HIGH_WATERMARK) {
        int newBucketSize = dict->bucketSize * 2 - 1;
        DictEntry** newBucket = calloc(1, sizeof(DictEntry*) * newBucketSize);

        int cnt = 0;
        for (int i = 0; i < dict->bucketSize; ++i) {
            for (DictEntry* e = dict->bucket[i]; e;) {
                DictEntry* detatched = e;
                e = e->next;
                detatched->next = NULL;

                int newSlot = detatched->hash % newBucketSize;
                if (newBucket[newSlot]) {
                    detatched->next = newBucket[newSlot];
                }
                newBucket[newSlot] = detatched;
                ++cnt;
            }
        }
        assert(cnt == dict->size);

        free(dict->bucket);
        dict->bucket = newBucket;
        dict->bucketSize = newBucketSize;
    }

    return true;
}

bool dict_erase(Dict* dict, char* key)
{
    DictEntry* found = dict_get_internal(dict, key);
    if (!found) {
        return false;
    }

    found->data = TOMBSTONE;
    return true;
}