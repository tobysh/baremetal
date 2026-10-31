---
title: Fixed-Size Pool
summary: Carve a buffer into equal slots and recycle them through a free list stored inside the free slots themselves, with O(1) alloc and free.
minutes: 30
difficulty: medium
tags: pool, free-list, allocator
id: c-allocators/fixed-pool
mode: harness
---
When every object has the same size (network packets, list nodes, particles), a **pool** beats a general heap: no search, no splitting, no fragmentation. The buffer is cut into equal **slots**, and the unused slots form a singly linked **free list**.

## The free list lives inside the free slots

A free slot holds no user data, so its first bytes can store the pointer to the next free slot. No extra memory is needed:

```text
free_head -> [slot 0 | next] -> [slot 1 | next] -> [slot 2 | NULL]
```

- `pool_alloc` pops the head: `p = free_head; free_head = *(void **)p;`
- `pool_free` pushes: `*(void **)p = free_head; free_head = p;`

Both are a few instructions. Because freed slots go to the **front**, the most recently freed slot is reused first (LIFO), which is also good for caches.

## Slot size

A slot must be big enough for a pointer and keep each slot 8-aligned. So `slot = align_up(max(objsize, sizeof(void *)), 8)` (use 8 for `sizeof(void *)` here) and `count = cap / slot`. Leftover bytes at the end are unused.

## Catching misuse

A pool cannot trust callers. `pool_free` should reject, returning -1 and changing nothing:

- a pointer outside the buffer,
- a pointer inside the buffer that is not the **start** of a slot (interior pointer),
- a slot that is already free (double free), which would otherwise create a cycle in the free list.

Finding a double free by walking the free list costs O(count). Real pools often use a bitmap or a debug-only check instead. Here correctness is the lesson, so walk the list.

## Limits

Freed slots are not cleared, and the allocator cannot know if you still use a pointer after freeing it. It also assumes the buffer you pass is 8-aligned.

## Your turn

Implement the four pool functions.
