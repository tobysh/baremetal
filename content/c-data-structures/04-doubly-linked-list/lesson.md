---
title: Doubly Linked List
summary: Add a prev pointer to every node so you can walk both ways and unlink a node in O(1). Practise careful pointer surgery at the head and tail.
minutes: 35
difficulty: medium
tags: linked-lists, pointers, data-structures
id: c-data-structures/doubly-linked-list
mode: harness
---
A singly linked list (see [Linked Lists](/learn/c-memory/linked-lists)) only knows the next node. To delete a node you must find its *predecessor*, which means walking from the head. A **doubly linked list** keeps both directions:

```c
typedef struct Node {
    int val;
    struct Node *prev;
    struct Node *next;
} Node;

typedef struct {
    Node *head;
    Node *tail;
    size_t len;
} DList;
```

Keeping `tail` as well as `head` makes appending at the back O(1) too. Given a pointer to any node, you can now unlink it in constant time.

## Unlinking a node

Four cases fold into one pattern: fix the neighbour on each side, and update `head` or `tail` when there is no neighbour.

```c
if (n->prev) n->prev->next = n->next; else list->head = n->next;
if (n->next) n->next->prev = n->prev; else list->tail = n->prev;
free(n);
list->len--;
```

## Inserting after a node

```c
Node *m = malloc(sizeof *m);
m->val = v;
m->prev = pos;
m->next = pos->next;
if (pos->next) pos->next->prev = m; else list->tail = m;
pos->next = m;
```

The order matters: read `pos->next` *before* you overwrite it.

## Reversing

To reverse, swap `prev` and `next` in every node, then swap `head` and `tail`.

## Invariants to keep in mind

- `head->prev == NULL` and `tail->next == NULL`.
- For every node `n`: `n->next->prev == n` and `n->prev->next == n`.
- An empty list has `head == tail == NULL` and `len == 0`.
- A one-node list has `head == tail`.

The hidden tests check all of these after every operation.

Implement the list operations below.
