---
title: Fit Policies
summary: Add best fit next to first fit, count the blocks each search examines, and compare the policies on the same workloads with deterministic counters.
minutes: 40
difficulty: hard
tags: heap, first-fit, best-fit, benchmarking
id: c-allocators/fit-policies
mode: harness
---
The free-list heap of the last lesson always took the **first** block that fit. That is one *policy*. When several free blocks could serve a request, which one should you pick? The choice changes how fast the search is and how the heap fragments over time.

## Two policies

- **First fit**: walk blocks in address order and take the first free one with `size >= need`. The search stops early, so it is usually cheap. Leftovers from splitting tend to pile up near the start of the heap, so later searches walk past them.
- **Best fit**: walk *all* blocks and remember the free one with the smallest `size >= need`. On a tie the lowest address wins, because only a strictly smaller block replaces the candidate. It leaves the smallest leftover, so big holes stay intact, but it always scans everything (this heap has no size-ordered index) and its tiny leftovers are often too small to be useful.

Both policies split the chosen block exactly as before, and both round the request up to a multiple of 16 first.

## Two kinds of waste

**Internal fragmentation** is waste *inside* a block you handed out: a 17-byte request occupies a 32-byte payload, so 15 bytes are lost to rounding. **External fragmentation** is free space that exists but is split into holes too small for a request. Fit policies cannot change rounding, but they decide which holes are consumed and which survive, so they steer external fragmentation.

## A trace where the policies diverge

In the diagrams below, `H` is a 16-byte header, `U` is 16 bytes of used payload and `.` is 16 bytes of free payload. The 2048-byte heap holds five blocks, then the first and third are freed, leaving holes of 512 and 256 bytes:

```text
H................................HUUUUH................HUUUUHUUUU...UUUU
^ hole A (512 free, 32 units)      ^ 64  ^ hole B (256)    ^ 64  ^ 1072 used (67 units, abbreviated)
```

Now request 200 bytes (rounded to 208), then 300 bytes (rounded to 304).

- **First fit** puts the 200 in hole A (first that fits). Hole A shrinks to 288 bytes. The 304 needs more than 288 and more than 256: **failure**, even though 544 bytes are free in total.
- **Best fit** puts the 200 in hole B (256 is the smallest hole that fits), keeping hole A whole. The 304 then fits in A: **both succeed**.

Reverse the hole order or change the sizes and first fit can win instead. A fair statement is "on this workload ...", never "best fit is better".

## Counters, not clocks

Wall-clock time depends on the machine and noise. Instead count events the algorithm defines:

- `search_steps`: how many blocks a search examined (used or free; every visited block counts once, including the chosen one).
- `failed_allocs`: requests with `n > 0` that returned `NULL`.
- Afterwards `free_blocks` and `largest_free` from `heap_stats`.

The same input always gives the same counters, so tests can demand exact numbers and you can derive them by hand. Here is the heart of the search, with the counter in the right place:

```c
for (Block *b = first_block(); b; b = next_block(b)) {
    steps++;                           // examined, whether usable or not
    if (!b->is_free || b->size < need) continue;
    if (policy == FIRST_FIT) return b; // stop at once
    /* best fit: keep the smaller candidate, keep scanning */
}
```

## What this lesson cannot tell you

These counters model a linked-list search on a 2 KiB heap with synthetic workloads. They ignore cache effects and real programs, and a real allocator's choice also depends on its data structures (size-segregated lists make "best fit" almost free). Whether you can *explain* a difference in the numbers is not machine-graded; the task asks you to write it as a comment.

## Your turn

The starter has a working first-fit heap. Finish `find_block` (counter plus best fit), then write `run_workload`, which replays an operation list under one policy and reports the counters.
