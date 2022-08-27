#ifndef __GENERIC_LIST_H__
#define __GENERIC_LIST_H__

typedef struct ListNode ListNode;
typedef struct List List;

struct ListNode {
    struct ListNode* prev;
    struct ListNode* next;
    // char[N] to store value
};

#define list_node_get(T, n) ((T*)((n) + 1))

struct List {
    struct ListNode* front;
    struct ListNode* back;
    int len;
};

List* list_new();
void list_clear(List* list);

#define list_is_empty(l) (l->len == 0)
#define list_len(l) (l->len)

void* _list_back(List* list);
void* _list_front(List* list);
void* _list_at(List* list, int idx);
#define list_back(T, l) ((T*)_list_back(l))
#define list_front(T, l) ((T*)_list_front(l))
#define list_at(T, l, i) ((T*)_list_at(l, i))

void _list_push_front(List* list, void* data, int size);
void _list_push_back(List* list, void* data, int size);
#define list_push_front(l, e) _list_push_front(l, ((void*)(&e)), sizeof(e))
#define list_push_back(l, e) _list_push_back(l, ((void*)(&e)), sizeof(e))

void list_pop_front(struct List* list);
void list_pop_back(struct List* list);

struct List* list_append(struct List* a, struct List* b);

#endif
