---
title: Condvar and a Blocking Queue
summary: Wait for a condition without spinning using Condvar, guard against spurious wakeups, and build a blocking queue with a timeout.
minutes: 30
difficulty: hard
tags: threads, condvar, mutex
id: rust-concurrency/condvar-queue
mode: harness
---
A consumer thread needs an item that a producer has not made yet. Two bad answers: loop and check (burns a whole CPU core), or sleep for a guessed time (slow, and still wrong sometimes). The right answer is a **condition variable**: `std::sync::Condvar` lets a thread sleep until another thread says "something changed".

A `Condvar` always works together with a `Mutex`, which protects the state being waited on:

```rust
let pair = Arc::new((Mutex::new(false), Condvar::new()));

// waiter
let (lock, cv) = &*pair;
let mut ready = lock.lock().unwrap();
while !*ready {
    ready = cv.wait(ready).unwrap();     // unlocks, sleeps, relocks on wake
}

// notifier (another thread)
let (lock, cv) = &*pair;
*lock.lock().unwrap() = true;
cv.notify_one();                         // or notify_all()
```

`wait` atomically releases the mutex and parks the thread, so the notifier can get the lock; when woken, the thread reacquires the lock before `wait` returns.

## Always wait in a loop

The operating system may wake a waiter for no reason (a **spurious wakeup**), or another consumer may have grabbed the item first. So never write `if queue.is_empty() { wait }`. Re-check the condition each time, which is what `while` does. The helper does it for you:

```rust
let guard = cv.wait_while(guard, |q| q.is_empty()).unwrap();   // returns once the predicate is false
```

## Timeouts

`wait_timeout_while(guard, dur, pred)` gives up after `dur`. It returns `(guard, WaitTimeoutResult)`; check `result.timed_out()` or just look at the state again.

## Modify, then notify

Change the shared state while holding the lock, then call `notify_one()`. A notification sent when nobody is waiting is lost, which is fine because a late waiter checks the condition first and finds the item already there. That is exactly why the loop-on-condition pattern is correct and a bare `wait()` is not.

## This lesson's exercise

Build `BlockingQueue<T>`: an unbounded FIFO queue where `pop` sleeps while it is empty, and `pop_timeout` gives up after a deadline. It must work shared in an `Arc` between many producers and consumers.
