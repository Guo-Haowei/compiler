#include "../src/generic/dict.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#define TEST_ARRAY_SIZE 24

void print_dict(Dict* dict)
{
    int entries = 0;
    printf("Dict: size: %d, cap: %d\n", dict->size, dict->bucketSize);
    for (int i = 0; i < dict->bucketSize; ++i) {
        printf("slot [%d]", i);
        for (DictEntry* e = dict->bucket[i]; e; e = e->next) {
            printf(" -> ('%s'(%u): %d)", e->key, e->hash, *((int*)(e->data)));
            ++entries;
        }
        printf("\n");
    }
    printf("-------------\n");
}

int main()
{
    printf("running dict test...\n");

    Dict* dict = dict_new();

    int arr[TEST_ARRAY_SIZE];
    for (int i = 0; i < TEST_ARRAY_SIZE; ++i) {
        arr[i] = i + 1;
    }

    // clang-format off
    char keys[TEST_ARRAY_SIZE][16] = {
        "auto", "int", "char", "main", "for", "break",
        "__VA_ARGS__", "__LINE__", "error", "signed", "register", "continue",
        "switch", "case", "include", "define", "warning", "do",
        "struct", "union", "return", "__FILE__", "unsigned", "while"
    };
    // clang-format on

    for (int i = 0; i < TEST_ARRAY_SIZE / 4; ++i) {
        bool ok = dict_try_add(dict, keys[i], &arr[i]);
        assert(ok);
    }
    print_dict(dict);

    for (int i = TEST_ARRAY_SIZE / 4; i < TEST_ARRAY_SIZE / 2; ++i) {
        bool ok = dict_try_add(dict, keys[i], &arr[i]);
        assert(ok);
    }
    print_dict(dict);

    for (int i = TEST_ARRAY_SIZE / 2; i < TEST_ARRAY_SIZE; ++i) {
        bool ok = dict_try_add(dict, keys[i], &arr[i]);
        assert(ok);
    }
    print_dict(dict);

    for (int i = 0; i < TEST_ARRAY_SIZE; ++i) {
        int* p = (int*)dict_get(dict, keys[i]);
        assert(p && *p == arr[i]);
    }

    dict_clear(dict);
    free(dict);

    printf("dict test passed\n");
    return 0;
}
