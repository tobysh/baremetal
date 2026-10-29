---
title: Channels
summary: Send values between threads with mpsc channels, clone senders for many producers, and let the channel closing end a receive loop.
minutes: 25
difficulty: medium
tags: threads, channels, mpsc
id: rust-concurrency/channels
mode: harness
---
"Do not communicate by sharing memory; share memory by communicating." A **channel** is a queue with two ends. `std::sync::mpsc` ("multiple producer, single consumer") gives you a `Sender<T>` and a `Receiver<T>`:

```rust
use std::sync::mpsc;
use std::thread;

let (tx, rx) = mpsc::channel();
thread::spawn(move || {
    tx.send(String::from("ping")).unwrap();
});
let msg = rx.recv().unwrap();     // blocks until something arrives
```

Sending **moves** the value. After `send`, the sender no longer owns it, so two threads can never touch the same message at once. That is how a whole class of data races is ruled out by the type system.

## When does a channel end?

`recv()` returns `Err` once every `Sender` has been dropped **and** the queue is empty. That makes this loop the idiomatic way to drain a channel:

```rust
for msg in rx {          // same as rx.iter(); stops when all senders are gone
    println!("{msg}");
}
```

The classic bug is a loop that never ends. You hold a `tx`, you clone it for each worker, and you forget that the **original** is still alive in the spawning function. Either `drop(tx)` after the spawn loop, or create the clones and move the original into the last worker.

## Many producers

Clone the sender, one clone per producer thread:

```rust
for id in 0..3 {
    let tx = tx.clone();
    thread::spawn(move || tx.send(id).unwrap());
}
drop(tx);                // the original must go too
```

Messages from different producers arrive in an unpredictable interleaving, so tests (and your own code) must not rely on cross-thread ordering. Messages from **one** sender do arrive in the order they were sent.

## Non-blocking receive

`rx.try_recv()` returns immediately with `Err(Empty)` or `Err(Disconnected)`, and `rx.try_iter()` yields whatever is waiting right now without blocking. `recv_timeout(d)` waits at most `d`.

## This lesson's exercise

Write `fan_in` (several producers into one consumer), `pipeline` (a worker thread that transforms what it receives), and `drain_ready` (a non-blocking drain).
