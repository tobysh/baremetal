---
title: Scoped Threads
summary: Borrow local data in threads with thread::scope, split a slice into disjoint mutable chunks, and drop the Arc and the 'static requirement.
minutes: 25
difficulty: medium
tags: threads, scope, borrowing
id: rust-concurrency/scoped-threads
mode: harness
---
`thread::spawn` demands a `'static` closure because the new thread might outlive the function that started it. That forces you to clone data or wrap it in `Arc`, even when you plainly intend to wait for the thread before leaving.

**`std::thread::scope`** gives the compiler that promise directly. Every thread spawned inside the scope is **joined automatically before `scope` returns**, so those threads may borrow from the surrounding stack:

```rust
let data = vec![1, 2, 3, 4];
let (left, right) = data.split_at(2);

let total = thread::scope(|s| {
    let a = s.spawn(|| left.iter().sum::<i32>());     // borrows `left`
    let b = s.spawn(|| right.iter().sum::<i32>());    // borrows `right`
    a.join().unwrap() + b.join().unwrap()
});
// `data` is still ours here
```

No `move`, no `Arc`, no clones. `s.spawn` returns a `ScopedJoinHandle` that works like the usual one. If you do not join a handle yourself, the scope joins it at the end, and if any thread panicked, `scope` itself panics after all have finished.

## Disjoint mutable borrows

Rust allows several `&mut` borrows if they point at **different** data. Slice methods carve a slice into non-overlapping pieces:

- `split_at_mut(mid)` gives two `&mut` halves.
- `chunks_mut(n)` yields `&mut` chunks of at most `n` items.

Hand one chunk to each scoped thread and they can all write at the same time with no lock, because the borrow checker has proved the chunks never overlap:

```rust
thread::scope(|s| {
    for chunk in data.chunks_mut(size) {
        s.spawn(move || {          // `move` moves the &mut chunk into the thread
            for x in chunk { *x += 1; }
        });
    }
});
```

(`move` here only moves the chunk reference, not the whole vector.)

Remember that `chunks` and `chunks_mut` panic if the chunk size is 0, and that an empty slice produces no chunks.

## When to use which

Use `scope` for fork-join work on data you own right now: parallel loops over a slice, splitting a computation in two. Use `spawn` plus channels or `Arc` for long-lived background threads that outlive the call.

## This lesson's exercise

Write `parallel_sum`, `increment_all` and `min_max` with scoped threads.
