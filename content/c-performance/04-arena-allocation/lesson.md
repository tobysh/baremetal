---
title: Arena Allocation
summary: Replace one system allocation per object with a bump-pointer arena. Count system calls, check alignment and overlap, and handle a failing allocator.
minutes: 30
difficulty: medium
tags: performance, allocation, arena, failure
id: c-performance/arena-allocation
mode: harness
---
A program that builds 1000 small objects with 1000 separate allocations pays for 1000 requests to the allocator, 1000 later releases, and bookkeeping for each block. When all the objects share one lifetime (a parse tree, one request, one frame) an **arena** does better: ask the system for a big **chunk**, hand out pieces by moving a pointer, and release everything at once.

## The counters

The file wraps the system allocator in `sys_alloc(n)` and `sys_free(p, n)`. They count calls and live bytes. They also return 4096-aligned memory, so a chunk starts on a page boundary, and they can be told to fail once (`sys_fail_next = 1`) to rehearse an out-of-memory error.

A general-purpose allocator also adds a header to every block. The size is not fixed in real life; **as a stated assumption** this lesson charges 16 bytes per allocation. An arena pays none per object, but wastes the unused tail of each chunk. Run the program: 1000 nodes of 16 bytes need 1000 system calls the naive way, 4 with the arena.

## The arena you build

```c
void *arena_alloc(arena_t *a, size_t n, size_t align);
void  arena_reset(arena_t *a);
void  arena_destroy(arena_t *a);
```

- Chunks are `CHUNK` (4096) bytes, or exactly `n` bytes for a request larger than `CHUNK`.
- The newest chunk is the only one that serves requests. Round the offset up to `align` (a power of two up to 64; chunk bases are page aligned), and bump it by `n`. If the request does not fit, **abandon the rest of the chunk** and get a new one. (Real arenas sometimes keep older chunks for reuse; we do not, to keep the model small.)
- If `sys_alloc` fails, or all `MAXCHUNKS` slots are used, return `NULL` and leave the arena exactly as it was, still usable.
- `arena_reset` keeps the first chunk, frees the others, and starts again from the beginning of the first chunk.
- `arena_destroy` frees every chunk.

## What the tests check

The tests contain their own model of this policy and compare `sys_allocs` with its prediction for random request sequences. They write a pattern into every block and verify none was overwritten, which catches overlap and off-by-one bugs, and they check alignment. A schedule with a failing allocator checks the error path. A request pattern can be unlucky: a chunk holding a 4000-byte request followed by a 200-byte one wastes 96 bytes and starts another. That waste is part of the tradeoff, not a bug.

Not covered: freeing single objects (an arena cannot), thread safety, and the real cost of page faults. The counters show how many requests were made, not how long the system took.
