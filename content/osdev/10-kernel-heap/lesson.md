---
title: The Kernel Heap
summary: Build kmalloc and kfree on a block list with first-fit search, splitting, coalescing and 16-byte alignment, plus a kcalloc that survives integer overflow.
minutes: 60
difficulty: hard
tags: memory, heap, allocator
id: osdev/kernel-heap
mode: harness
---
Frames come in 4096-byte lumps. A kernel needs thousands of small objects: process descriptors, buffers, strings, file nodes. It needs its own `malloc`, called **kmalloc** by tradition. You write one here, and you have already met everything it needs.

## The design: a list of blocks

The heap is one contiguous **arena** of memory. We cut it into **blocks**, each with a small header followed by the usable bytes (the *payload*):

```
+--------+-----------------+--------+--------------+--------+---------+
| header | payload (used)  | header | payload(free)| header | payload |
+--------+-----------------+--------+--------------+--------+---------+
```

The header says how large the payload is, whether the block is free, and where the next block is. The blocks form a **linked list in address order**, with each block starting exactly where the previous one ends.

```c
typedef struct Block {
    size_t size;           // payload bytes, not counting the header
    int free;
    struct Block *next;
} __attribute__((aligned(16))) Block;
```

The `aligned(16)` makes `sizeof(Block)` 32, a multiple of 16, so the payload that follows each header is 16-byte aligned too. That matters: the CPU, SSE instructions and `malloc` convention all expect 16.

## kmalloc

1. A request of 0 bytes returns `NULL`.
2. **Round the size up** to a multiple of 16.
3. **First fit:** walk the list and take the first free block that is big enough.
4. **Split** it if the leftover is worth keeping: if `block->size >= wanted + sizeof(Block) + 16`, carve a new free block out of the tail and link it in. Otherwise hand over the whole block, because a leftover with no room for a header plus a useful payload would be lost anyway.
5. Mark the block used and return the address just after its header (`block + 1`).
6. If nothing fits, return `NULL`.

## kfree

Mark the block free, found with `(Block *)ptr - 1`. Then **coalesce**: walk the list and merge every pair of neighbouring free blocks into one. Without that, a heap fragments: freeing three adjacent 100-byte blocks would leave three pieces and be unable to serve a 250-byte request. Merging means adding the second block's header and payload to the first's size and unlinking it.

`kfree(NULL)` must do nothing.

## kcalloc and overflow

`kcalloc(count, size)` returns zeroed memory for `count` items. The danger is `count * size` wrapping around: with 64-bit values, `count = 2^62` and `size = 4` multiply to 0, so a naive version allocates almost nothing and the caller then writes 2^62 items into it. This is a real, classic vulnerability. Check `size != 0 && count > SIZE_MAX / size` before multiplying and return `NULL` if it fails.

## A worked trace

Use a 256-byte arena and `sizeof(Block) == 32`. The header takes 32 bytes, so the one free block has `size = 256 - 32 = 224`.

```
heap_init:
 [H 224 free ..........................................]

kmalloc(40)   -> rounds to 48; 224 >= 48 + 32 + 16, so it splits
 [H 48 used ][H 144 free ......................]

kmalloc(100)  -> rounds to 112; 144 >= 112 + 32 + 16 is false (160 > 144), so no split
 [H 48 used ][H 144 used ........................]    <- 144 bytes handed out for a 100-byte request
```

That last step is deliberate. The leftover would be only 32 bytes (144 - 112), enough for a header and nothing else. A block you can never use is waste, so the allocator keeps it attached to the allocation.

```
kfree(first)   -> [H 48 free ][H 144 used ]
kfree(second)  -> both free and adjacent: merge into one block of 48 + 32 + 144 = 224
 [H 224 free ..........................................]
```

The merge arithmetic matters: the second block's **header** becomes ordinary payload, which is why you add `sizeof(Block)`.

## Why 16 bytes

Alignment isn't decoration. On x86-64 some instructions fault on an unaligned operand (for example SSE's `movaps`), and the ABI promises that anything `malloc` returns is aligned for any type, which in practice means 16. Compilers rely on it, and when the promise is broken the failure shows up much later in unrelated code, like the stack misalignment bug from the context switch lesson.

## Fragmentation

Even a correct allocator can lose memory without leaking any. **External fragmentation** is when free memory exists but is split into holes too small for a request: ten 100-byte holes can't serve one 500-byte allocation. Coalescing fights it by merging neighbours as soon as they are free. **Internal fragmentation** is the unused tail inside an allocation, such as the rounding to 16 and the unsplittable leftover above. First fit is simple and decent; best fit leaves smaller leftovers but searches the whole list. Real kernels often use **slab allocators**: for objects of one popular size (say, process structs), they carve whole frames into equal slots, so there's no fragmentation within that class at all.

## The heap you will build later

Notice what this allocator does *not* do. It doesn't ask for more memory when it runs out: your frame allocator and page mapper from the last two lessons could grow the arena by mapping more frames at its end. A real `kmalloc` calls a function like `heap_grow()` when no block fits. You have all the pieces.

## Debugging checklist

- **`kmalloc` returns overlapping pointers:** the split formula is off. The new block starts at `(uint8_t *)(b + 1) + want`, not at `b + want`, because the header sits in front of the payload.
- **Memory is "lost" after free:** coalescing only looked one block ahead. Loop until no adjacent pair is both free, and don't advance after a merge.
- **A crash in `kfree`:** you freed a pointer that didn't come from `kmalloc`, or freed the same one twice. The header is just the 32 bytes in front of the pointer, so garbage there is garbage everywhere.
- **Guard bytes changed:** you wrote past a block's payload, or `heap_init` didn't align or shrink the size correctly.
- **`kcalloc` returns a huge block for a nonsense request:** the multiplication wrapped. Compare `count > SIZE_MAX / size` *before* multiplying.

## Your turn

The tests give you an arena. Your `heap_init` must align the start up to 16 and carve it into a single free block. The tests also put guard bytes around the arena to catch writes outside it.
