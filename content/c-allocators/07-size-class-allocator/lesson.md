---
title: Capstone: Route Small Allocations
summary: Combine bounded small-object pools with a general heap and route frees by ownership.
minutes: 40
difficulty: hard
id: c-allocators/size-class-allocator
---
Real programs allocate mostly small objects, and the general heap handles them badly: every allocation walks the block list, pays a 16-byte header, and leaves tiny holes that coalescing may not repair. A **size-class allocator** puts the common small sizes in dedicated pools and keeps the general heap for everything else. This is the idea behind the small-object bins in `malloc` implementations such as jemalloc and tcmalloc.

## The design

| Request size | Served from | Slot size | Slots |
|---|---|---|---|
| 1 to 16 bytes | small pool | 16 | 8 |
| 17 to 64 bytes | large pool | 64 | 8 |
| 65 bytes or more | general heap | any | the heap buffer |
| pool full | general heap | any | the heap buffer |

Each pool is a static array plus one `used` flag per slot: `small_pool[8][16]`, `small_used[8]`, and the same for 64 bytes. They live *outside* the heap buffer, so pool objects have no header and the heap never sees them.

## Trade-offs

- **Internal fragmentation.** A pool slot is always full size. A 12-byte request wastes 4 bytes in a 16-byte slot, and a 17-byte request wastes 47 in a 64-byte slot. Few, coarse classes waste more; many classes waste less but need more pools.
- **No external fragmentation inside a pool.** Every slot is the same size, so any free slot serves any request of that class. Freed slots are reused directly and need no coalescing.
- **Speed.** Finding a slot scans 8 flags, independent of how fragmented the heap is. The general heap still walks blocks.
- **Fixed capacity.** When a pool is full, the request must not fail. It falls back to `heap_alloc`, which costs a header and a search but still succeeds.

## A worked example

Fresh state, then three calls. `S` marks a used small slot, `L` a used large slot, `.` a free one:

```text
small pool: ........   large pool: ........   heap: 1 free block
classes_alloc(12)   ->  S.......               (slot 0, lowest free)
classes_alloc(40)   ->  large: L.......        (17..64 uses the 64-byte pool)
classes_alloc(200)  ->  heap_alloc(200)        (pools untouched, heap gets a used block)
```

Free the 12-byte pointer and the next small request gets slot 0 again, since the lowest free slot is always chosen. The 200-byte pointer is not in any pool, so its free goes to `heap_free`.

## Routing a free by ownership

`classes_free(p)` receives no size, so the *address* decides who owns it. Compare it as an integer range:

```c
uintptr_t x = (uintptr_t)p, lo = (uintptr_t)small_pool;
if (x >= lo && x < lo + 8 * 16) {
    size_t off = x - lo;
    if (off % 16 || !small_used[off / 16]) return -1;  // interior or already free
    small_used[off / 16] = 0;
    return 0;
}
```

Three outcomes matter: a pointer inside a pool must be an exact slot start *and* currently allocated; a bad pool pointer returns -1 and changes nothing; any other pointer is the heap's problem and `heap_free`'s return value is passed through. Never test a slot with `used[i]` when you meant `!used[i]`; the two callers would share memory.

## Limits

One thread, fixed pools, no `realloc`, and no way to detect a pointer from an older `classes_init` (callers must not do that). A production allocator uses many classes, per-thread caches and page-level ownership maps.

## Your turn

The supplied heap, pool arrays and flags are in the starter. Write `classes_init`, `classes_alloc` and `classes_free`.
