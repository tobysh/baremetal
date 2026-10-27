---
title: Linked Lists
summary: Build a chain of heap nodes with structs and pointers. Learn struct, typedef, the -> operator, push_front, traversal, reversing a list, and freeing every node.
minutes: 30
difficulty: hard
id: c-memory/linked-lists
mode: harness
---
An array is one solid block: growing it means finding a bigger block and copying. A **linked list** is the opposite: a chain of small separate pieces, each knowing where the next one lives. Think of a treasure hunt where every clue tells you the location of the next clue.

## Structs

A **struct** bundles several values into one type:

```c
struct Point {
    int x;
    int y;
};

struct Point p = {3, 4};
printf("%d\n", p.x);      // 3, the dot reaches a field
```

`typedef` gives a type a shorter name, so you can drop the word `struct` everywhere.

## A node

Each piece of the chain is a **node**: a value plus a pointer to the next node.

```c
typedef struct Node {
    int value;
    struct Node *next;
} Node;
```

(Inside the definition we must write `struct Node *` because the name `Node` does not exist yet.) The last node's `next` is `NULL`, meaning "end of the list". The list as a whole is represented by a pointer to its first node, the **head**. An empty list is just `head == NULL`.

## The arrow operator

If `n` is a *pointer* to a node, you reach its fields with `->`:

```c
Node *n = malloc(sizeof *n);
n->value = 7;      // shorthand for (*n).value = 7;
n->next = NULL;
```

Use `.` on a struct itself and `->` on a pointer to a struct.

## Adding to the front

Making the new node the head takes three steps: allocate it, point its `next` at the old head, and return it as the new head.

```c
Node *push_front(Node *head, int value) {
    Node *n = malloc(sizeof *n);
    if (n == NULL) return head;   // allocation failed: list unchanged
    n->value = value;
    n->next = head;
    return n;
}
```

The caller writes `head = push_front(head, 5);`. Pushing 1, then 2, then 3 gives the list 3 -> 2 -> 1.

## Walking the list

```c
size_t count = 0;
for (const Node *cur = head; cur != NULL; cur = cur->next) {
    count++;
}
```

Stop when `cur` is `NULL`. Never dereference `NULL`.

## Reversing

Walk the list once, and flip each node's `next` to point backwards. You need to remember the next node before overwriting the link, or you lose the rest of the chain:

```c
Node *prev = NULL;
while (cur != NULL) {
    Node *next = cur->next;   // save the rest
    cur->next = prev;         // flip the arrow
    prev = cur;               // move forward
    cur = next;
}
// prev is the new head
```

## Freeing

Every node was a separate `malloc`, so every node needs its own `free`. Read `next` **before** freeing the node, since after `free` its memory is off limits.

## Common mistakes

- Dereferencing `NULL` when the list is empty.
- Freeing a node and then reading `node->next`.
- Losing the head pointer and leaking the entire list.
