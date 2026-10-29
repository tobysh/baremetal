---
title: Channels and Worker Pipelines
summary: Send results from worker threads over mpsc channels, drop the spare Sender so the receiver finishes, and make unordered arrivals deterministic.
minutes: 30
difficulty: medium
tags: threads, channels, mpsc
id: rust-intermediate/channel-pipelines
mode: harness
---
A **channel** moves values between threads. `mpsc::channel()` gives a `(Sender, Receiver)` pair: *multiple producers, single consumer*. Sending **moves** the value, so the two threads never share it, and there is nothing to lock.

```rust
use std::sync::mpsc;
use std::thread;

let (tx, rx) = mpsc::channel();
for id in 0..3 {
    let tx = tx.clone();                    // each producer gets its own Sender
    thread::spawn(move || tx.send(id * 10).unwrap());
}
drop(tx);                                   // drop the original!
let mut all: Vec<i32> = rx.iter().collect();
```

## When does the receiver stop?

`rx.iter()` (and `for x in rx`) ends when **every** `Sender` has been dropped. A thread's clone is dropped when the thread ends. But the original `tx` in the spawning function would live to the end of the function, so the loop would wait forever. Forgetting the `drop(tx)` is the classic channel deadlock.

## Order is not guaranteed, except...

Messages from *different* senders arrive in whatever order the threads happen to run. That is not a bug but you must not depend on it. Two ways to stay deterministic:

1. **Tag and sort.** Send `(index, value)` and sort by index after collecting.
2. **Use a guarantee.** Messages from a *single* sender arrive in the order they were sent.

Tests for concurrent code must be deterministic: never use `sleep` to hope threads finish. Channels and `join` are the synchronization.

## Pipelines

A pipeline chains stages with channels: stage 1 reads `rx1` and writes `tx2`, stage 2 reads `rx2` and writes `tx3`, and so on. Each stage ends when its input closes; dropping the first sender shuts the whole chain down in order.

## This lesson's exercise

Write `fan_out_sum`, `squares_in_order`, `collect_from_producers` and `pipeline`.
