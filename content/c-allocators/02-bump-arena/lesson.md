---
title: Bump Arena
summary: Allocate by moving a pointer forward, align every request, free everything at once with reset, and roll back to a mark.
minutes: 30
difficulty: medium
tags: arena, bump-allocator, alignment
id: c-allocators/bump-arena
mode: harness
---
The simplest allocator that is actually useful is the **bump arena**. It owns one buffer and one number, `off`, the count of bytes handed out so far. To allocate you align the next free address, move `off` past the new object, and return the address. There is no per-object free: you throw the whole arena away with `reset`, or roll back to an earlier point.

```text
base                                       base+cap
|--used--|pad|--obj--|        free          |
                      ^ off
```

## Why anyone uses it

- Allocation is a few instructions and cannot fragment.
- Everything with the same lifetime (one request, one frame, one parse) is released by a single `reset`.
- Objects allocated together sit next to each other, which is kind to caches.

The price: you cannot free one object, and a long-lived arena that is never reset only grows.

## Alignment relies on the address, not the offset

The buffer you are given need not be aligned itself. Suppose `base` ends in `...0x1003` and you want 8-byte alignment. Offset 0 is *not* aligned. You have to align `base + off`, so the padding depends on the real address. Lesson 1's `padding_for` is exactly this computation.

## Fail cleanly

A failed allocation must return `NULL` and **leave the arena unchanged**. Two ways to get that wrong:

1. Moving `off` first and checking afterwards.
2. Checking `off + pad + size <= cap` with a `size` so large that the sum wraps around to a small number. Compare against the remaining room instead: `size <= cap - off - pad`, after making sure `pad <= cap - off`.

## Marks and rewinding

`arena_mark` returns the current `off`. `arena_rewind(a, mark)` sets `off` back, releasing everything allocated since, like popping a stack. A mark that is *ahead* of the current offset is stale or forged, so it is ignored.

## Limits of this lesson

A rewind does not clear memory, and pointers into rewound space still look valid to the compiler. Using one is a bug the allocator cannot catch. This arena also does no locking: it is for one thread.

## Your turn

Implement the six functions. The starter's `main` allocates a few objects and prints the offsets.
