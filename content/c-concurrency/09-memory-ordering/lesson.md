---
title: Memory Ordering
summary: Publish data from one thread to another with a release store and an acquire load, and build a lock with just those two orderings.
minutes: 35
difficulty: hard
tags: threads, atomics, memory-model
id: c-concurrency/memory-ordering
mode: harness
---
In [the last lesson](/learn/c-concurrency/atomics) every atomic call used the default, the strictest ordering. Now see what that ordering is for and when you can ask for less.

## Why order matters

Compilers and CPUs reorder memory operations to go faster. On a single thread you never notice, because the result is the same. Between threads you can. Consider:

```c
// thread A                   // thread B
data = 42;                    while (!ready) {}
ready = 1;                    print(data);       // could print 0 !
```

Nothing stops the CPU from making `ready = 1` visible before `data = 42`, and nothing stops B from reading `data` early. (This is also a data race on plain variables.) What we need is a way to say: *everything I did before this write must be visible to anyone who sees this write.*

## Release and acquire

That is exactly a **release** store paired with an **acquire** load:

```c
// thread A
data = 42;                                              // ordinary write
atomic_store_explicit(&ready, 1, memory_order_release);

// thread B
while (atomic_load_explicit(&ready, memory_order_acquire) == 0) {}
print(data);                                            // guaranteed to see 42
```

- **release** on a store: no earlier read or write may be moved after it. Everything before it is "published".
- **acquire** on a load: no later read or write may be moved before it. If it sees the value the release wrote, then it also sees everything written before that release.

The pair forms a *happens-before* edge, and that is all you need for handing data from one thread to another.

## The orderings

- `memory_order_relaxed`: atomic, but no ordering at all. Good for plain counters.
- `memory_order_acquire` / `memory_order_release`: the pair above. `memory_order_acq_rel` is both, for read-modify-write operations.
- `memory_order_seq_cst`: the default. One global order that all threads agree on. Safest, and sometimes slower.

A mutex is exactly acquire on lock and release on unlock: that is why everything done before one thread unlocks is visible to the next thread that locks. A spinlock needs only those two.

Be aware that x86 hides many ordering bugs: it already behaves almost like acquire/release for plain loads and stores. The tests pass there even if you pick the wrong order, but the same code can break on ARM or on a different compiler. Choose the orderings by reasoning, not by trying.

## Your turn

Write a one-shot mailbox using release/acquire, then a spinlock that names its orderings explicitly.
