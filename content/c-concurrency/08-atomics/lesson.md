---
title: C11 Atomics
summary: Use atomic integers, fetch-add, compare-and-swap and an atomic_flag spinlock to share data between threads without a mutex.
minutes: 30
difficulty: medium
tags: threads, atomics, lock-free
id: c-concurrency/atomics
mode: harness
---
A mutex works by making threads wait. C11 gives you something lighter: **atomic** operations. An atomic operation on a variable happens as one indivisible step. No other thread can ever see it half done, and it is never a data race.

```c
#include <stdatomic.h>

atomic_long counter = 0;            // an atomic long
atomic_fetch_add(&counter, 1);      // counter += 1, indivisible; returns the OLD value
long now = atomic_load(&counter);   // read it
atomic_store(&counter, 0);          // write it
```

Counting with `atomic_fetch_add` needs no mutex at all, and the total is always exact. It is also quicker than lock, add, unlock.

## Compare-and-swap

The key building block of lock-free code is **compare-and-swap** (CAS):

```c
bool atomic_compare_exchange_strong(atomic_int *obj, int *expected, int desired);
```

It means: *"if `*obj` still equals `*expected`, set it to `desired` and return true. Otherwise copy the current value into `*expected` and return false."* All in one indivisible step.

CAS turns "read, decide, write" into something safe, as long as you **retry** when it fails. For example, to keep the maximum of many values:

```c
int old = atomic_load(target);
while (old < v && !atomic_compare_exchange_weak(target, &old, v)) {
    // somebody else changed it; "old" now holds the fresh value, so look again
}
```

`_weak` may fail spuriously, which is fine inside a loop and can be faster on some CPUs. `_strong` does not fail spuriously. Use `_weak` in loops.

## A spinlock from a flag

`atomic_flag` is the simplest atomic: a boolean that is guaranteed lock-free. `atomic_flag_test_and_set` sets it and returns the **previous** value, in one step. That is enough to build a lock:

```c
while (atomic_flag_test_and_set(flag))
    ;                         // it was already set: someone holds the lock, spin
// ... critical section ...
atomic_flag_clear(flag);      // release
```

A spinlock burns CPU while it waits, so use it only for very short critical sections. It is a teaching tool here, and real code usually prefers a mutex.

## Your turn

Use all three tools. By default these functions use the strongest ordering (`seq_cst`), which is what you want until the next lesson explains the alternatives.
