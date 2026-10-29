---
title: Shared State: Arc, Mutex and Lock Order
summary: Share a collection of Mutex-protected values between threads with Arc, and avoid deadlock by always locking in the same order.
minutes: 35
difficulty: medium
tags: threads, mutex, arc, deadlock
id: rust-intermediate/shared-bank
mode: harness
---
Channels move data. Sometimes threads must instead share one piece of data and change it in place. Two types make that safe:

- `Arc<T>` is an atomically reference-counted pointer: cloning it gives another owner of the same `T`, and the value is freed when the last owner goes.
- `Mutex<T>` allows only one thread at a time inside; `lock()` returns a guard that gives access and unlocks when it is dropped.

```rust
let counter = Arc::new(Mutex::new(0));
let handles: Vec<_> = (0..4).map(|_| {
    let c = Arc::clone(&counter);
    thread::spawn(move || { *c.lock().unwrap() += 1; })
}).collect();
for h in handles { h.join().unwrap(); }
assert_eq!(*counter.lock().unwrap(), 4);
```

The tests wait with `join`, never with `sleep`. Joining is what makes the result certain.

## Locking two things: deadlock

A bank transfer must lock **two** accounts. Suppose thread A transfers from account 0 to 1 while thread B transfers from 1 to 0. If each locks its *source* first, A holds 0 and waits for 1 while B holds 1 and waits for 0: **deadlock**, and both wait forever.

The cure is a global lock order: always lock the account with the **smaller index first**, whichever is the source. Then no two threads can ever wait on each other in a circle.

```rust
let (lo, hi) = if from < to { (from, to) } else { (to, from) };
let mut first  = self.accounts[lo].lock().unwrap();
let mut second = self.accounts[hi].lock().unwrap();
```

Afterwards decide which guard is the source and which is the destination.

## Sharing the whole Bank

The `Bank` itself holds `Vec<Mutex<i64>>`. Its methods take `&self`, not `&mut self`, because the mutexes provide the mutability. That is what lets many threads share one `Arc<Bank>`.

## This lesson's exercise

Implement `Bank::new`, `balance`, `total`, `deposit`, `transfer` and `parallel_deposits`. Hidden tests hammer the bank from many threads in opposite directions; a wrong lock order will hang.
