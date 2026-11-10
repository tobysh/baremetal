---
title: Read a Fragmentation Map
summary: Render heap headers, allocated payloads and free payloads without changing heap state.
minutes: 40
difficulty: hard
id: c-allocators/fragmentation-map
---
"The heap has 400 bytes free" does not tell you whether a 200-byte request will succeed. It needs **one contiguous hole**, and free space can be scattered. A **fragmentation map** makes that visible: draw the heap one character per 16-byte unit.

- `H` for each block header (16 bytes, the allocator's overhead),
- `U` for each 16 bytes of *used* payload,
- `.` for each 16 bytes of *free* payload.

Every block is `H` followed by `size / 16` copies of `U` or `.`. Payload sizes in this heap are always multiples of 16, so the division is exact.

## A worked example

A fresh 256-byte heap is one free block with a 240-byte payload: 15 units.

```text
H...............
```

Allocate `a = heap_alloc(32)` and `b = heap_alloc(16)`. The first block is split into 32 bytes, the second into 16, and the remainder stays free (240 - 32 - 16 - 16 - 16 = 160 bytes, 10 units). Both allocations are in use:

```text
HUUHUH..........
```

Free `a`. It cannot merge with `b` (still used), so a hole appears in front of it:

```text
H..HUH..........
```

Total free payload is 2 + 10 = 12 units, but the largest hole is 10. Now free `b`: it merges with the free block before and after it and the map collapses to one block again:

```text
H...............
```

## Reading a map

- Many `H` characters mean overhead: every block costs one header.
- A long run of `.` between two `H` characters is a hole. The longest such run is the biggest request that can succeed, and it can be much smaller than the total number of `.`.
- Alternating `HU` and `H.` is external fragmentation. A heap that keeps allocating and freeing small blocks drifts towards this shape.

Compare the two measures in `heap_stats`: `free_bytes` counts all `.` units times 16, `largest_free` counts only the longest run. The gap between them is the fragmentation.

## The contract: measuring and writing

`heap_map` is a *snprintf-style* function. It returns the number of characters the full map needs (excluding the NUL), and writes as much of it as fits:

```c
char small[8];
size_t need = heap_map(small, sizeof small);   // need == 16, small holds "H......" + NUL
```

So a caller can pass `cap == 0` and `NULL` to measure first, then allocate `need + 1` bytes and call again. Keep counting after the buffer is full; only stop *writing*.

The function only reads the heap. It must not change blocks, counters or bytes beyond `cap`. This is a map of the teaching heap, not a profiler for a real process.

## Your turn

Walk the blocks with `first_block()` / `next_block()` (supplied) and emit the characters. Handle the three buffer cases: `cap == 0`, a buffer too small, and a buffer that is big enough.
