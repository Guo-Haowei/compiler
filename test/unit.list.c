#include "../src/generic/list.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static void print_list(List* list)
{
    printf("[");
    for (ListNode* c = list->front; c; c = c->next) {
        printf("%d,", *(int*)(c + 1));
    }
    printf("]\n");
}

static void list_eq(List* list, int arrLen, int* arr)
{
    assert(list_len(list) == arrLen);

    print_list(list);
    for (int i = 0; i < arrLen; ++i) {
        assert(*list_at(int, list, i) == arr[i]);
    }
    {
        int i = 0;
        for (ListNode* n = list->front; n; n = n->next, ++i) {
            assert(*list_node_get(int, n) == arr[i]);
        }
    }
    {
        int i = list->len - 1;
        for (ListNode* n = list->back; n; n = n->prev, --i) {
            assert(*list_node_get(int, n) == arr[i]);
        }
    }
}

#define LIST_EQ(LIST, LEN, ...)         \
    {                                   \
        int __array[LEN] = __VA_ARGS__; \
        list_eq(LIST, LEN, __array);    \
    }                                   \
    ((void)0)

int list_test()
{
    printf("running list test...\n");
    List* l = list_new();
    assert(list_is_empty(l));
    assert(list_len(l) == 0);

    int arr[5] = { 0, 1, 2, 3, 4 };
    list_push_back(l, arr[0]);
    list_push_back(l, arr[1]);
    list_push_back(l, arr[2]);

    LIST_EQ(l, 3, { 0, 1, 2 });

    list_push_front(l, arr[3]);
    list_push_front(l, arr[4]);

    LIST_EQ(l, 5, { 4, 3, 0, 1, 2 });

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
    LIST_EQ(l, 3, { 3, 0, 1 });

    v = list_front(int, l);
    assert(*v == 3);
    v = list_back(int, l);
    assert(*v == 1);

    list_clear(l);
    assert(list_is_empty(l));

    free(l);

    printf("list test passed\n");
    return 0;
}

#ifndef NO_MAIN
int main()
{
    return list_test();
}
#endif
