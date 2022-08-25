#include "list.h"

#include "common.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static ListNode* _list_node_new(int size)
{
    ListNode* node = calloc(1, sizeof(ListNode) + ALIGN(size, 16));
    node->prev = NULL;
    node->next = NULL;
    return node;
}

List* list_new()
{
    List* list = calloc(1, ALIGN(sizeof(List), 16));
    list->front = NULL;
    list->back = NULL;
    list->len = 0;
    return list;
}

void list_delete(List* plist)
{
    assert(plist);
    list_clear(plist);
    free(plist);
}

void list_clear(List* list)
{
    assert(list);

    while (list->len) {
        list_pop_back(list);
    }

    assert(list->len == 0);
    assert(list->front == 0);
    assert(list->back == 0);
}

void* _list_back(List* list)
{
    assert(list && list->len && list->front && list->back);
    return list->back + 1;
}

void* _list_front(List* list)
{
    assert(list && list->len && list->front && list->back);
    return list->front + 1;
}

void* _list_at(List* list, int idx)
{
    assert(list && idx >= 0 && idx < list->len);

    ListNode* n = list->front;
    for (; idx--; n = n->next) { }
    return n + 1;
}

void _list_push_front(List* list, void* data, int size)
{
    assert(list);
    ListNode* n = _list_node_new(size);
    n->prev = NULL;
    n->next = list->front;
    memcpy(n + 1, data, size);
    if (list->len == 0) {
        list->back = n;
    } else {
        list->front->prev = n;
    }

    list->front = n;
    ++list->len;
}

void _list_push_back(List* list, void* data, int size)
{
    assert(list);
    ListNode* n = _list_node_new(size);
    n->next = NULL;
    n->prev = list->back;
    memcpy(n + 1, data, size);
    if (list->len == 0) {
        list->front = n;
    } else {
        list->back->next = n;
    }

    list->back = n;
    ++list->len;
}

void list_pop_front(List* list)
{
    assert(list);
    assert(list->len);
    ListNode* n = list->front;
    list->front = n->next;
    if (n->next) {
        n->next->prev = NULL;
    } else {
        list->back = NULL;
    }

    --list->len;
    free(n);
}

void list_pop_back(List* list)
{
    assert(list);
    assert(list->len);
    ListNode* n = list->back;
    list->back = n->prev;
    if (n->prev) {
        n->prev->next = NULL;
    } else {
        list->front = NULL;
    }

    --list->len;
    free(n);
}

List* list_append(List* a, List* b)
{
    a->back->next = b->front;
    b->front->prev = a->back;
    b->front = a->front;
    a->len += b->len;

    memset(b, 0, sizeof(List));
    return a;
}
