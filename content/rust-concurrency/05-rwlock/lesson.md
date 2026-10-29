---
title: RwLock
summary: Allow many readers or one writer with RwLock, update a shared map atomically under a write lock, and probe locks with try_read.
minutes: 25
difficulty: medium
tags: threads, rwlock, hashmap
id: rust-concurrency/rwlock
mode: harness
---
A `Mutex` lets one thread in at a time, even if all of them only want to **read**. A `RwLock<T>` ("reader-writer lock") relaxes that rule:

- any number of threads may hold a **read** guard at once (`read()`), or
- exactly one thread may hold a **write** guard (`write()`), with no readers at the same time.

This is the same rule Rust applies to references: many `&T`, or one `&mut T`. `RwLock` enforces it at runtime across threads.

```rust
let config = Arc::new(RwLock::new(HashMap::new()));

{
    let mut w = config.write().unwrap();   // exclusive
    w.insert("theme", "dark");
}                                          // guard dropped, lock released

let r = config.read().unwrap();            // shared; many threads may do this
println!("{:?}", r.get("theme"));
```

Pick `RwLock` when reads far outnumber writes and each read takes real time. If critical sections are tiny, a plain `Mutex` is often as fast and simpler, because an `RwLock` has more bookkeeping.

## Read-modify-write needs the write lock

Reading under a read lock and then writing under a write lock is the same check-then-act bug as in the previous lesson, and worse: you cannot upgrade a read guard to a write guard while holding it, and trying to take `write()` while you hold `read()` on the same thread deadlocks. For an increment, take the **write** lock once and use the map's `entry` API:

```rust
*map.write().unwrap().entry(key.to_string()).or_insert(0) += 1;
```

## try_read and try_write

`try_read()` and `try_write()` never block. They return `Err(TryLockError::WouldBlock)` if the lock is not available right now. They are useful for "skip this if busy" logic and for tests.

## This lesson's exercise

Build a thread-safe `Registry` of counters: shared handles, `get`, `set`, an atomic `incr`, and a sorted `snapshot`. Then write `can_read_now`.
