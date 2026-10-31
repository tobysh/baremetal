---
title: Free-List Heap
summary: Build a general allocator over a static buffer with block headers, first fit, splitting of oversized blocks and coalescing of neighbours on free.
minutes: 45
difficulty: hard
tags: heap, free-list, coalescing, splitting
id: c-allocators/free-list-heap
mode: harness
---
Pools need equal sizes. A general heap serves any size, in any order, so it must remember where every block starts and how big it is. The classic answer is a **header** in front of each block.

```text
|hdr|payload (used)|hdr|payload (free)      |hdr|payload (used)|
```

Blocks tile the buffer with no gaps: the next block starts at `this + HDR + this->size`. The header here is 16 bytes (`size`, `is_free`), so payloads stay 16-aligned if the buffer is, and every payload size is a multiple of 16.

## Allocate: find, split, mark

1. Round the request up to a multiple of 16.
2. Walk the blocks in address order and take the **first** free block that is big enough (**first fit**).
3. **Split**: if the block is larger than needed by at least a new header plus the minimum payload (`HDR + 16 = 32` bytes), cut it in two and leave the rest free. Otherwise hand out the whole block. Splitting only when the remainder is useful avoids headers wrapped around 0-byte payloads.

## Free: mark and coalesce

Mark the block free, then merge with free neighbours. Without that step the heap shatters into small free blocks that can never serve a larger request (**external fragmentation**). Merging with the next block is easy. To merge with the previous block you need to know where it is: this heap finds it by walking from the start (O(n)). Real allocators add a footer or a "previous size" field (**boundary tags**) to find it in O(1).

If `a`, `b`, `c` are adjacent and you free them in the order `a`, `c`, `b`, the last free must merge in **both** directions and leave one block.

## Rejecting bad frees

`heap_free(NULL)` does nothing and returns 0. A pointer that is not exactly a payload start of a used block (interior pointer, foreign pointer, already freed) returns -1 and changes nothing.

## Overhead

Every allocation costs one header. Sixteen 16-byte allocations use `16 * (16 + 16)` bytes, so a 1024-byte heap serves exactly 32 of them. Plus rounding: a 17-byte request occupies 32 payload bytes. Those are the bounds the tests check.

## Limits

This is a teaching heap: one thread, one buffer, linear searches, no `realloc`. It never moves blocks, and it cannot detect writes past a block's end (compare the guard bytes of the debugging course).

## Your turn

`heap_init`, `heap_stats` and the block walkers are given. Write `heap_alloc` and `heap_free`. The challenge adds in-place `heap_realloc`.
