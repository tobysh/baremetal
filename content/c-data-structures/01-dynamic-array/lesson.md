---
title: Dynamic Array
summary: Build a growable array of ints with malloc and realloc. Learn length versus capacity, why doubling makes appends cheap, and how to insert in the middle.
minutes: 30
difficulty: medium
tags: heap, arrays, data-structures
id: c-data-structures/dynamic-array
mode: harness
---
A C array has a fixed size. When you do not know in advance how many items you will store, you need an array that can **grow**. Almost every language has one built in (`std::vector`, Python's `list`, Rust's `Vec`). In C you build it yourself.

## Length and capacity

The trick is to keep two numbers:

- `len`: how many items are really stored.
- `cap`: how many items the allocated block *could* hold.

```c
typedef struct {
    int *data;
    size_t len;
    size_t cap;
} IntVec;
```

Appending is cheap while `len < cap`: write at `data[len]` and add one. When the block is full, ask `realloc` for a bigger one. `realloc(ptr, bytes)` returns a block of the new size holding the old contents (it may move them), and returns `NULL` on failure, leaving the old block valid.

```c
int *bigger = realloc(v->data, new_cap * sizeof *bigger);
if (bigger == NULL) return -1;   // keep v->data as it was
v->data = bigger;
v->cap = new_cap;
```

Never write `v->data = realloc(v->data, ...)` directly: on failure you would lose the only pointer to the old block.

## Why double?

If you grew by one slot each time, every append would copy the whole array: n appends cost about n²/2 copies. If you **double** the capacity when full, copies happen rarely (at 4, 8, 16, ...) and the total cost over n appends is under 2n. We say append is *amortised* constant time.

## Inserting in the middle

To insert at index `i`, everything from `i` onward must shift one slot right. Use `memmove` (not `memcpy`) because source and destination overlap:

```c
memmove(&v->data[i + 1], &v->data[i], (v->len - i) * sizeof v->data[0]);
v->data[i] = x;
v->len++;
```

## Common mistakes

- Reading `data[len]`: valid memory, but not a stored item.
- Forgetting that a fresh vector has `cap == 0` and `data == NULL`. `realloc(NULL, n)` works like `malloc(n)`, which is handy.
- Using `memcpy` for overlapping ranges.
- Multiplying by `sizeof(int)` in some places and not others. `sizeof *ptr` stays right if the type changes.

Implement the vector operations below.
