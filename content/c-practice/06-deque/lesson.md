---
title: A Double-Ended Queue
summary: Build a doubly linked list with malloc, then use it as a deque you can add to and remove from at both ends, reverse and tidy up.
minutes: 40
difficulty: medium
tags: data-structures, heap
id: c-practice/deque
mode: harness
---
In [Linked Lists](/learn/c-memory/linked-lists) you built a singly linked list: each node knew only the **next** one. That makes the front easy and the back awkward. A **doubly linked list** gives every node two links, `prev` and `next`, so you can walk either way, and by remembering both the `head` and the `tail` you can work at either end instantly.

A list used this way is a **deque** ("deck"): a double-ended queue. Customers can join or leave from the front or the back.

```c
typedef struct DNode {
    int value;
    struct DNode *prev;
    struct DNode *next;
} DNode;

typedef struct {
    DNode *head;     // first node, or NULL
    DNode *tail;     // last node, or NULL
    int size;
} Deque;
```

```
NULL <- [1] <-> [2] <-> [3] -> NULL
         ^head              ^tail
```

## The rules that keep it healthy

For every node, `node->next->prev` must be `node` and `node->prev->next` must be `node`. For an empty deque, `head` and `tail` are both `NULL` and `size` is `0`. Every operation must leave all of this true. Most bugs in linked lists are one forgotten link.

## Adding at the back

```c
DNode *n = malloc(sizeof(DNode));
if (n == NULL) return -1;
n->value = value;
n->next = NULL;
n->prev = d->tail;
if (d->tail != NULL) d->tail->next = n;   // link the old last node forward
else d->head = n;                          // list was empty: n is also the head
d->tail = n;
d->size++;
```

Pushing at the front is the mirror image: swap `head`/`tail` and `next`/`prev`.

## Removing

Removing from an end: remember the node, move `head` (or `tail`) to its neighbour, set that neighbour's link back to `NULL` (or set both ends to `NULL` if it was the last node), `free` the node, lower `size`.

Removing from the **middle** is where the doubly linked list shines: the neighbours simply point at each other, `n->prev->next = n->next` and `n->next->prev = n->prev`, then you free `n`. Handle the cases where `n` is the head or tail separately, since one side is `NULL`.

## Reversing

Walk the list; for each node swap its `prev` and `next`; finally swap `head` and `tail`. Be careful to read `next` *before* you swap, because after swapping it points backwards.

## Clean up

Every `malloc` needs a `free`. `deque_free` walks the list, saving each `next` before freeing the node, then resets the deque to empty so it can be used again.

## Your turn

Write all the operations. The tests check both directions of every link, so a half-updated list will be caught.
