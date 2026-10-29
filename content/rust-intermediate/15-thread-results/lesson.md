---
title: Results and Panics Across Threads
summary: Return Result from threads, turn a panicking thread into an Err, share a closure with Arc, and join every thread so the first failure is reproducible.
minutes: 40
difficulty: hard
tags: threads, errors, panics, join
id: rust-intermediate/thread-results
mode: harness
---
`thread::spawn` returns a `JoinHandle<T>`, and `join()` gives back `Result<T, Box<dyn Any + Send>>`. That outer `Result` is about the thread itself: `Ok(value)` if it finished normally, `Err(payload)` if it **panicked**. If the thread's own work can fail, `T` is a `Result` too, so `join()` returns a nested `Result<Result<T, E>, _>`.

```rust
match thread::spawn(work).join() {
    Ok(Ok(v)) => /* finished and succeeded */,
    Ok(Err(e)) => /* finished, work failed with e */,
    Err(_)    => /* the thread panicked */,
}
```

## Reading a panic

The payload is a `Box<dyn Any + Send>`. A panic with a literal (`panic!("boom")`) carries a `&'static str`; a panic with formatting (`panic!("bad {}", 3)`) carries a `String`. Recover the message with `downcast_ref`:

```rust
if let Some(s) = payload.downcast_ref::<&str>() { s.to_string() }
else if let Some(s) = payload.downcast_ref::<String>() { s.clone() }
else { "unknown panic".to_string() }
```

A panic in a spawned thread does not take the program down; it only fails that thread's `join`. The default panic message still prints on stderr, which is expected.

## Sharing one closure between threads

To run the same function on many threads, wrap it in `Arc` and clone the `Arc` into each thread. The closure type needs `Send + Sync + 'static`: `Sync` because several threads call it at once.

## Deterministic failures

If three workers fail, which error do you report? "Whichever happened first in time" changes from run to run. Make it deterministic: **join every thread in index order** and keep the first error you meet. That is the lowest-index failure. Joining *all* of them, even after you have an error, also means no thread is left running when you return.

## This lesson's exercise

Write `run_guarded`, `collect_ordered` and `parse_chunks`. Hidden tests count how many workers actually ran, so returning early without joining everyone will be caught.
