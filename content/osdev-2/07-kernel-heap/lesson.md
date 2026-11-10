---
title: A Small Kernel Heap
summary: Allocate aligned blocks from a fixed arena and coalesce neighbours on free.
minutes: 50
difficulty: medium
tags: kernel, systems
id: osdev-2/kernel-heap
mode: harness
---
The [frame allocator](/learn/osdev-2/physical-frames) hands out memory 4096 bytes at a time. Most kernel objects are far smaller: a task structure is a couple of hundred bytes, a pipe is a few dozen, a list node is sixteen. Giving each one a whole frame would waste over 95% of RAM. A **heap** takes a large region (an **arena**) and divides it into variable-sized **blocks**, so `kmalloc(24)` costs about 24 bytes.

This lesson builds the core of such an allocator: **first-fit** allocation with **splitting**, and **coalescing** on free. Those three ideas are what `malloc` in every C library is built on, and their failure modes (fragmentation, double frees, frees of interior pointers) are the bugs you will be chasing in every kernel you ever write.

## Blocks and descriptors

The arena is 256 bytes. Its state is a list of **descriptors**, one per block, kept in address order. Each says where the block starts, how long it is, and whether it is in use:

```c
typedef struct { size_t off, len; int busy; } Block;
static Block blocks[32];     /* blocks[0..count-1] are valid */
static unsigned count;
```

Two rules hold at all times, and every function must preserve them:

1. **The blocks tile the arena exactly.** Block `i + 1` starts where block `i` ends (`blocks[i+1].off == blocks[i].off + blocks[i].len`), the first starts at 0 and the last ends at 256. No gaps, no overlaps.
2. **No two neighbouring blocks are both free.** If they were, they should have been merged into one.

After `heap_init` there is a single free block covering everything:

```text
[ free 0..256 ]                      count = 1
```

Real allocators usually store the descriptor in a small **header** just before each block, inside the arena itself. Keeping descriptors in a separate array, as here, lets you look at the algorithm without pointer arithmetic on headers, and it means a buggy caller cannot overwrite the metadata by writing past the end of its block.

## Rounding and alignment

Every request is rounded up to a multiple of 8. That keeps every block offset a multiple of 8, and since the arena itself is 8-aligned, every pointer returned is suitable for a `uint64_t` or a pointer. The usual trick rounds up with a mask:

```c
n = (n + 7) & ~(size_t)7;      /* 1..8 -> 8, 9..16 -> 16, 24 -> 24 */
```

Check the size **before** rounding. A request of `(size_t)-1` plus 7 wraps around to 6, which rounds to 0, and an allocator that rounds first would happily "succeed" with a zero-byte block. Anything over 256 cannot fit anyway, so reject it first; zero is also rejected, because `heap_alloc(0)` returning a pointer would invite the caller to write to it.

## First fit and splitting

To allocate `n` bytes, walk the blocks in address order and take the **first** free block whose length is at least `n`. If the block is exactly `n` bytes, just mark it busy. If it is larger, **split** it: the front `n` bytes become the busy block and the rest becomes a new free block right after it.

```text
heap_alloc(9)   rounds to 16
before:  [ free 0..256 ]
after:   [ busy 0..16 ][ free 16..256 ]                   returns arena + 0

heap_alloc(24)
after:   [ busy 0..16 ][ busy 16..40 ][ free 40..256 ]   returns arena + 16

heap_alloc(16)
after:   [ busy 0..16 ][ busy 16..40 ][ busy 40..56 ][ free 56..256 ]
```

Splitting inserts a descriptor into the middle of the array, so everything after position `i` shifts up by one, and `count` grows. If the array is already full, the allocation must fail **before** anything changes. (With a 256-byte arena, 8-byte granularity and 32 descriptors this can't actually happen: 32 blocks of at least 8 bytes already fill 256 bytes. Keep the check anyway; it is what stops a resized arena from writing past the array.)

First fit is simple and in practice fragments memory no worse than cleverer policies such as best fit. Its weakness is speed: the walk is linear in the number of blocks.

## Freeing and coalescing

`heap_free(p)` must find the block that **starts** exactly at `p` and is busy. Anything else is a caller bug and gets -1: a pointer into the middle of a block, a pointer outside the arena, `NULL`, or a block that is already free (a double free). Finding the block by searching the descriptor list makes all of those checks automatic.

Once the block is marked free, rule 2 may be broken. Repair it by merging with the **predecessor** if it is free, then with the **successor** if it is free. Merging means adding one block's length to the other and deleting the absorbed descriptor (shifting the rest down).

```text
Three 80-byte blocks, then free the middle one and the first:

start:          [ A busy 0..80 ][ B busy 80..160 ][ C busy 160..240 ][ free 240..256 ]
heap_free(B):   [ A busy 0..80 ][ free 80..160   ][ C busy 160..240 ][ free 240..256 ]
heap_free(A):   [ free 0..160                    ][ C busy 160..240 ][ free 240..256 ]
                  A merged with its free successor B
heap_alloc(160) returns arena + 0 again
```

Without coalescing the heap would hold two free 80-byte blocks side by side, and `heap_alloc(160)` would fail even though 160 contiguous bytes are free. That is **fragmentation**, and coalescing is the cure for the part of it that is self-inflicted.

Only merge **neighbours**. In the picture above, the free block at 80..160 and the one at 240..256 are not adjacent; C sits between them, and merging them would hand out C's bytes to someone else.

## Common mistakes

- **Merging the wrong way round.** After merging with the predecessor, the freed block *is* the predecessor now. If you then check "the next block" using the old index, you skip one.
- **Changing state, then failing.** A split that bumps `count` before discovering the array is full leaves a half-made descriptor behind. Do every check first.
- **Accepting interior pointers.** Comparing `p` against each block's range instead of each block's start lets `heap_free(b + 1)` free `b`, and the caller's next write lands in memory that is now someone else's.

## Your turn

The arena and descriptor array are given. Implement `heap_init`, `heap_alloc` and `heap_free` with the rules above.
