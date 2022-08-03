#ifndef __LIST_H__
#define __LIST_H__
#include <stdint.h>

#define ALIGN(x, a) (((x) + (a)-1) & ~((a)-1))

typedef struct ListNode {
    struct ListNode* prev;
    struct ListNode* next;
    // char[N] to store value
} ListNode;

typedef struct List {
    struct ListNode* front;
    struct ListNode* back;
    int len;
} List;

struct List* list_new();
void list_delete(struct List* plist);
void list_clear(struct List* list);

#define list_is_empty(l) (l->len == 0)
#define list_len(l) (l->len)

void* _list_back(struct List* list);
void* _list_front(struct List* list);
void* _list_at(struct List* list, int idx);
#define list_back(T, l) ((T*)_list_back(l))
#define list_front(T, l) ((T*)_list_front(l))
#define list_at(T, l, i) ((T*)_list_at(l, i))

void _list_push_front(struct List* list, void* data, size_t size);
void _list_push_back(struct List* list, void* data, size_t size);
#define list_push_front(l, e) _list_push_front(l, ((void*)(&e)), sizeof(e))
#define list_push_back(l, e) _list_push_back(l, ((void*)(&e)), sizeof(e))

void list_pop_front(struct List* list);
void list_pop_back(struct List* list);

void list_print(struct List* list, void (*fptr)(void*));

#endif
