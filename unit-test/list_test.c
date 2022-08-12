#include "../src/generic/list.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>


static void print_list(struct List* list)
{
    printf("list(size:%d) [ ", list->len);
    for (struct ListNode* c = list->front; c; c = c->next) {
        printf("%d ", *(int*)(c + 1));
    }
    printf("]\n");
}


static int list_eq(struct List* list, int argc, ...)
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
        if (!list_eq(list, len, __VA_ARGS__)) {                   \
            printf("test failed on line %d\nexpect: ", __LINE__); \
            printf("[ " #__VA_ARGS__ ", ]\n");                    \
            printf("actual: ");                                   \
            assert(0);                                            \
        }                                                         \
    }

int list_test()
{
    printf("running list test\n");
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
    print_list(l);

    list_push_front(l, arr[3]);
    list_push_front(l, arr[4]);

    LIST_EQ(l, 5, 4, 3, 0, 1, 2);
    print_list(l);

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
    print_list(l);

    v = list_front(int, l);
    assert(*v == 3);
    v = list_back(int, l);
    assert(*v == 1);

    list_clear(l);
    assert(list_is_empty(l));

    list_delete(l);

    printf("list test passed\n\n");
    return 0;
}

#ifdef TEST_STANDALONE
int main()
{
    return list_test();
}
#endif
