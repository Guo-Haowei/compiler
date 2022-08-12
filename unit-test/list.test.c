#include "../src/generic/list.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(_MSC_VER)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

static void printInt(void* n) { printf("%d, ", *(int*)n); }

static void printList(struct List* list)
{
    printf("[ ");
    list_print(list, printInt);
    printf("]\n");
}

static int listEq(struct List* list, int argc, ...)
{
    va_list args;

    if (argc != list_len(list)) {
        return 0;
    }

    int* nums = malloc(sizeof(int) * argc);
    va_start(args, argc);
    for (int i = 0; i < argc; i++) {
        nums[i] = va_arg(args, int);
    }
    va_end(args);

    {
        int* p = nums;
        for (const struct ListNode* n = list->front; n; n = n->next, ++p) {
            if (*(int*)(n + 1) != *p) {
                free(nums);
                return 0;
            }
        }
    }

    {
        int* p = nums + argc - 1;
        for (const struct ListNode* n = list->back; n; n = n->prev, --p) {
            if (*(int*)(n + 1) != *p) {
                free(nums);
                return 0;
            }
        }
    }

    {
        for (int i = 0; i < argc; ++i) {
            int a = *list_at(int, list, i);
            if (a != nums[i]) {
                free(nums);
                return 0;
            }
        }
    }

    free(nums);
    return 1;
}

#define LIST_EQ(list, len, ...)                                   \
    {                                                             \
        if (!listEq(list, len, __VA_ARGS__)) {                    \
            printf("test failed on line %d\nexpect: ", __LINE__); \
            printf("[ " #__VA_ARGS__ ", ]\n");                    \
            printf("actual: ");                                   \
            printList(list);                                      \
            assert(0);                                            \
        }                                                         \
    }

int main()
{
    struct List* l = list_new();

    assert(ALIGN(1, 16) == 16);
    assert(ALIGN(8, 16) == 16);
    assert(ALIGN(15, 16) == 16);
    assert(ALIGN(16, 16) == 16);
    assert(ALIGN(18, 16) == 32);
    assert(ALIGN(31, 16) == 32);
    assert(ALIGN(32, 16) == 32);

    int arr[] = { 0, 1, 2, 3, 4 };
    list_push_back(l, arr[0]);
    list_push_back(l, arr[1]);
    list_push_back(l, arr[2]);

    LIST_EQ(l, 3, 0, 1, 2);

    list_push_front(l, arr[3]);
    list_push_front(l, arr[4]);

    LIST_EQ(l, 5, 4, 3, 0, 1, 2);

    int* v = NULL;
    v = list_front(int, l);
    assert(*v == 4);
    v = list_back(int, l);
    assert(*v == 2);
    v = list_at(int, l, 1);
    assert(*v == 3);
    v = list_at(int, l, 2);
    assert(*v == 0);

    list_pop_back(l);
    list_pop_front(l);
    LIST_EQ(l, 3, 3, 0, 1);

    v = list_front(int, l);
    assert(*v == 3);
    v = list_back(int, l);
    assert(*v == 1);

    list_clear(l);
    assert(list_is_empty(l));

    list_delete(l);
    printf("Ok\n");

#if defined(_MSC_VER)
    _CrtDumpMemoryLeaks();
#endif
}
