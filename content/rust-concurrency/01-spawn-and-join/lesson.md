---
title: Spawning and Joining Threads
summary: Start threads with thread::spawn, wait for them with join, collect their results, and survive a thread that panics.
minutes: 20
difficulty: easy
tags: threads, join
id: rust-concurrency/spawn-and-join
mode: harness
---
A **thread** is an independent flow of execution inside your program. `std::thread::spawn` takes a closure, starts running it on a new thread, and immediately returns a `JoinHandle`:

```rust
use std::thread;

let handle = thread::spawn(|| {
    println!("hello from another thread");
    6 * 7
});
let answer = handle.join().unwrap(); // waits, then gives back 42
```

`join()` blocks until the thread finishes and returns whatever the closure returned. Until you join, you have no idea whether the thread has run, is running, or is done. If `main` returns first, the process exits and any thread still running is simply killed, so always join the threads whose work you need.

## join returns a Result

`join` gives `Result<T, Box<dyn Any + Send>>`. It is `Err` when the thread **panicked**. A panic in a thread unwinds only that thread; the rest of the program carries on and you decide what to do:

```rust
match thread::spawn(|| -> i32 { panic!("oops") }).join() {
    Ok(v) => println!("got {v}"),
    Err(_) => println!("the worker panicked"),
}
```

Calling `.unwrap()` on that `Err` re-panics in the joining thread, which is often exactly what you want.

## Many threads, results in order

Spawn first, join afterwards. If you join inside the same loop that spawns, you run one thread at a time. Collect the handles into a `Vec`, then join them in order:

```rust
let handles: Vec<_> = (0..4).map(|i| thread::spawn(move || i * 10)).collect();
let results: Vec<i32> = handles.into_iter().map(|h| h.join().unwrap()).collect();
```

The `move` keyword gives each closure its own copy of `i`. The next lesson explains why it is needed; for now, keep it where the starter shows it.

## Thread identity

`thread::current().id()` returns a `ThreadId`. It is `Copy`, comparable, and unique per thread, which makes it a handy way to prove that code ran somewhere else.

## This lesson's exercise

Write `sum_chunks` (one thread per chunk, results in input order), `run_guarded` (turn a panicking worker into an `Err`) and `on_new_thread` (run a function on a fresh thread and report which thread ran it).
