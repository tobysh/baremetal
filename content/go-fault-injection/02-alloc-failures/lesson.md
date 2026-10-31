---
title: Allocation Failures at Every Boundary
summary: Inject an out-of-memory error at the 1st, 2nd, 3rd... allocation and make sure a failing function returns an error and leaks nothing.
minutes: 30
difficulty: medium
tags: error-handling, cleanup, fault-injection
id: go-fault-injection/alloc-failures
mode: harness
---
Go's garbage collector hides most leaks, but many programs manage resources that it cannot: buffer pools, arenas, file handles, memory-mapped regions, slots in a fixed table. A function that acquires several of them in sequence has a **boundary** between each pair of acquisitions, and a failure at any boundary must undo everything acquired before it.

## Counting boundaries

The technique is simple. Run the function once with a counting allocator to learn it makes `N` calls. Then run it `N` more times, failing call number `k` for `k = 1..N`. Every run must satisfy the same invariants:

- it returns an error (and `errors.Is(err, ErrNoMem)` still works),
- it returns no result,
- the allocator reports **zero live allocations**: each successful `Alloc` was matched by exactly one `Free`.

Hand-written tests tend to try only "the first allocation fails". The bug is usually at the second or third.

```go
type Allocator interface {
    Alloc(n int) ([]byte, error)
    Free(b []byte)
}
```

## The function

`Pack` stores a list of strings compactly: all bytes concatenated in a `data` block, plus an `index` block of 4-byte big-endian **end offsets**, one per item. It makes exactly three allocations in this order:

1. `data`, sized as the total length of all items,
2. `index`, sized `4 * len(items)`,
3. `scratch`, 4 bytes, used to encode each offset before it is copied into `index`. It is temporary: free it before returning, on success as well.

On success, the caller owns `data` and `index` and releases them with `Release`. On failure, `Pack` has already freed everything it got.

## Limits of this model

The fake allocator fails by call count. Real allocators fail by size and by system state; sizes here are small and unbounded growth is not tested. Go's own `make` never returns an error, so this pattern applies to resources you wrap, not to the language's heap.
