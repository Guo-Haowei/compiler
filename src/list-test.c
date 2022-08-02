#include "list.h"

#include <assert.h>
#include <stdio.h>

static void printInt(void* n)
{
    printf("%d ", *(int*)n);
}

static void printList(struct List* list)
{
    printf("[ ");
    list_print(list, printInt);
    printf("]\n");
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

    printList(l); // expect [ 0 1 2 ]

    list_push_front(l, arr[3]);
    list_push_front(l, arr[4]);

    printList(l); // expect [ 4 3 0 1 2 ]

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
    printList(l); // expect [ 3 0 1 ]

    v = list_front(int, l);
    assert(*v == 3);
    v = list_back(int, l);
    assert(*v == 1);

    list_clear(l);
    assert(list_is_empty(l));

    list_delete(l);
}
