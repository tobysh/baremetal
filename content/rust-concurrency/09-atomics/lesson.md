---
title: Atomics and Ordering
summary: Use AtomicUsize, AtomicBool and friends for lock-free counters, ids and stop flags, and choose between Relaxed and Release/Acquire ordering.
minutes: 30
difficulty: hard
tags: threads, atomics, memory-ordering
id: rust-concurrency/atomics
mode: harness
---
A mutex protects a *region of code*. For a single integer or flag that is heavy: a lock, a possible sleep, a guard to hold. **Atomic types** (`AtomicUsize`, `AtomicU64`, `AtomicI64`, `AtomicBool`, ...) are shared values whose individual operations the CPU performs indivisibly. They live in `std::sync::atomic`, are `Sync`, and are usually shared through an `Arc` or a `static`.

```rust
use std::sync::atomic::{AtomicUsize, Ordering};

static NEXT_ID: AtomicUsize = AtomicUsize::new(1);

fn new_id() -> usize {
    NEXT_ID.fetch_add(1, Ordering::Relaxed)   // returns the OLD value
}
```

Operations take `&self`, not `&mut self`, so a plain `static` works with no `unsafe` and no `static mut`. Useful methods:

- `load(ord)` / `store(v, ord)`: read or write.
- `fetch_add`, `fetch_sub`, `fetch_max`, `fetch_min`, `fetch_or`, ...: read-modify-write in one step; they return the previous value.
- `swap(v, ord)`; `compare_exchange(current, new, success, failure)`: write only if the value is still `current`. Loop on it to build your own atomic updates.

A plain `x.load(..)` followed by `x.store(x + 1, ..)` is **not** atomic as a pair; two threads can both load the same number. Use one `fetch_*` call.

## What is Ordering for?

Besides the value itself, a CPU and compiler may reorder *other* memory accesses around an atomic one. `Ordering` says how much freedom they get.

- **`Relaxed`**: only this atomic's own value is guaranteed. Perfect for counters and ids where nobody uses the number to decide whether *other* data is ready.
- **`Release` (on a store) and `Acquire` (on a load)**: they pair up. If thread A writes some data and then does `store(true, Release)`, and thread B sees `true` from a `load(Acquire)`, then B is guaranteed to see everything A wrote before the store. This is how a flag publishes data.
- **`SeqCst`**: the strongest, a single global order for all `SeqCst` operations. The right default when you are unsure; you pay a little speed for easier reasoning.

```rust
// Thread A                          // Thread B
DATA.store(42, Relaxed);             while !READY.load(Acquire) {}
READY.store(true, Release);          assert_eq!(DATA.load(Relaxed), 42);
```

With `Relaxed` on the flag instead, thread B would be allowed to see `READY` and still read an old `DATA`. Tests rarely catch this on x86 hardware, which is why the rule matters: correctness must come from the ordering, not from "it worked here".

## A stop flag

A background loop checks an `AtomicBool` each round, and the controlling thread flips it. A `store(true, Release)` and `load(Acquire)` pair is the usual choice if the worker then reads any data the controller wrote; for a pure "please stop" signal `Relaxed` is also sound.

## This lesson's exercise

Write four small atomic utilities: a counter, a global id generator, a running maximum and a stoppable worker.
