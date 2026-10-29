---
title: Arc and Mutex
summary: Share mutable state between threads with Arc<Mutex<T>>, keep check-and-update inside one lock, and recover from a poisoned mutex.
minutes: 25
difficulty: medium
tags: threads, mutex, arc
id: rust-concurrency/arc-mutex
mode: harness
---
Sometimes messages are the wrong tool and several threads genuinely need to update one value. Two types make that safe.

## Arc: shared ownership across threads

`Rc<T>` counts owners but its counter is not thread-safe, so Rust forbids sending it to another thread. **`Arc<T>`** ("atomically reference counted") is the thread-safe version. `Arc::clone(&a)` bumps the count; the value is freed when the last `Arc` drops. An `Arc` only gives you **shared, read-only** access.

## Mutex: one at a time

`Mutex<T>` wraps the value and hands it out only through `lock()`, which blocks until nobody else holds it:

```rust
let counter = Arc::new(Mutex::new(0));
let handles: Vec<_> = (0..4).map(|_| {
    let counter = Arc::clone(&counter);
    thread::spawn(move || {
        for _ in 0..1000 {
            *counter.lock().unwrap() += 1;
        }
    })
}).collect();
for h in handles { h.join().unwrap(); }
assert_eq!(*counter.lock().unwrap(), 4000);
```

`lock()` returns a `MutexGuard`. The lock is held until the guard is **dropped**, at the end of its scope or of the statement for a temporary. Unlike C, you cannot touch the data without the lock, and you cannot forget to unlock.

## Keep the check and the update together

A very common bug is releasing the lock between looking and acting:

```rust
if *balance.lock().unwrap() >= 20 {      // lock taken and dropped here
    *balance.lock().unwrap() -= 20;      // another thread may have spent it meanwhile
}
```

Each statement is safe on its own, but the pair is not. Take **one** guard and keep it: `let mut g = balance.lock().unwrap(); if *g >= 20 { *g -= 20; }`.

## Poisoning

If a thread panics while holding the lock, the mutex is **poisoned**: later `lock()` calls return `Err(PoisonError)`, because the data may be half-updated. `.unwrap()` would panic too. If you can tolerate the data, the error still carries the guard: `lock().unwrap_or_else(|e| e.into_inner())`.

## This lesson's exercise

Implement `count_up`, a small `Account` whose `withdraw` never overdraws even when many threads hammer it, and `lock_even_if_poisoned`.
