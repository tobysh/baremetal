---
title: calloc, realloc and Growing a Buffer
summary: Get zeroed memory with calloc, resize a block with realloc, and use both to build a growable array that doubles its capacity as it fills.
minutes: 30
difficulty: medium
tags: heap, malloc, realloc
id: c-memory/calloc-realloc
mode: harness
---
`malloc` hands you memory full of leftover garbage, and you must decide the size up front. Two more functions from `<stdlib.h>` cover the other common needs.

## calloc: zeroed memory

```c
int *counts = calloc(n, sizeof *counts);
```

`calloc(count, size)` reserves `count * size` bytes and sets every one of them to zero. It is the right call for counters, flags, and anything that should start out empty. It also checks that `count * size` does not overflow, which `malloc(n * size)` does not. Like `malloc`, it returns `NULL` on failure.

## realloc: resize a block

```c
int *bigger = realloc(old, new_size);
```

`realloc` makes a block larger or smaller and keeps the contents (up to the smaller of the two sizes). It may extend the block in place or move it to a new address, copying the data for you and freeing the old block. Two consequences:

- **Always use the returned pointer.** The old pointer may now be invalid.
- **Never write `p = realloc(p, n)`.** If `realloc` fails it returns `NULL` and leaves the old block untouched, but you have just overwritten your only pointer to it: a leak. Use a temporary:

```c
int *tmp = realloc(p, new_size);
if (tmp == NULL) {
    return -1;        // p is still valid; the caller can free it
}
p = tmp;
```

`realloc(NULL, n)` behaves like `malloc(n)`, which is handy for the very first allocation.

## A growable array

A fixed array cannot grow, but a pointer plus two numbers can: the **length** (how many items are stored) and the **capacity** (how many fit before more room is needed).

```c
typedef struct {
    int *data;
    size_t len;
    size_t cap;
} IntVec;
```

When `len == cap` and you want to add an item, grow the block. Growing by one slot each time would copy the data over and over. Instead **double the capacity** (starting from a small number such as 4). Most pushes then cost nothing, and the occasional copy is paid for by all the cheap pushes that follow. This is how the dynamic arrays in most languages work.

When you are finished adding, you can give back the unused tail with `realloc` to the exact size.

## Common mistakes

- `p = realloc(p, n)` leaking on failure.
- Using the old pointer after `realloc` succeeded.
- Resizing to `cap` instead of `cap * sizeof(int)` bytes.
- Reading memory beyond `len`; it holds garbage (after `malloc` or `realloc`) even though it is allocated.

## Your turn

Write a zeroed-array helper and the two key operations of a growable int array. The test starts with an empty `IntVec` (`data` is `NULL`, `len` and `cap` are 0).
