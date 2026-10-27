---
title: A Growable Array
summary: Build a vector that grows on demand with realloc: a struct holding a pointer, a length and a capacity, with push, pop and free.
minutes: 30
difficulty: hard
tags: heap, data-structures
id: c-systems/dynamic-array
mode: harness
---
A C array has a fixed size. But a program often does not know in advance how many items it will meet: lines in a file, players in a game, search results. You need an array that can **grow**.

The trick is to keep three facts together in a struct:

```c
typedef struct {
    int *data;       // the heap block holding the items
    size_t len;      // how many items are in use
    size_t cap;      // how many items the block has room for
} IntVec;
```

Think of a bookshelf. `cap` is how many slots the shelf has, `len` is how many slots hold a book. When the shelf is full and you buy another book, you do not buy a shelf one slot bigger. You buy a shelf *twice* the size, move your books over, and now you have plenty of spare room.

## realloc

`realloc(ptr, new_size)` resizes a heap block, copying the old contents across if it has to move. It also accepts `NULL`, in which case it behaves like `malloc`, which is handy for the very first push.

```c
int *bigger = realloc(v->data, new_cap * sizeof *bigger);
if (bigger == NULL) {
    return -1;            // v->data is still valid; do not lose it!
}
v->data = bigger;
v->cap = new_cap;
```

Notice we store the result in a *temporary* first. If `realloc` fails it returns `NULL` but leaves the old block alone. Writing `v->data = realloc(v->data, ...)` directly would overwrite your only pointer to that block with `NULL` and leak it.

## Why double?

If you grew by one slot each time, 1000 pushes would copy the array about 500,000 times in total. Doubling means each item is copied only a handful of times on average, so a push is cheap "on average". Nearly every growable container (C++ `vector`, Rust `Vec`, Python lists) does a version of this.

## The shape of push

```c
int vec_push(IntVec *v, int x) {
    if (v->len == v->cap) {
        // out of room: grow (start at 4 if cap is 0, otherwise double)
    }
    v->data[v->len] = x;
    v->len++;
    return 0;
}
```

Return `0` for success and `-1` for failure, a very common C convention.

## Your turn

The struct is written for you. Implement initialising, pushing, popping and freeing. The tests push a thousand numbers and check every one of them is still where it should be.
