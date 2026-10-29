---
title: A Thread Pool
summary: Build a fixed-size thread pool from channels, Arc<Mutex<Receiver>> and boxed closures, and shut it down cleanly in Drop.
minutes: 40
difficulty: hard
tags: threads, channels, pool, drop
id: rust-concurrency/thread-pool
mode: harness
---
Spawning a thread per task costs memory and start-up time, and nothing stops a flood of tasks from creating thousands of threads. A **thread pool** starts a fixed number of workers once and feeds them jobs through a queue. You now have every piece needed to build one.

## The design

```text
          +------------+   job   +----------+
execute --> Sender<Job> |-------->| Receiver |<-- shared by all workers
          +------------+         +----------+
                                   |   |   |
                              worker0 worker1 worker2
```

- A **job** is a boxed closure: `type Job = Box<dyn FnOnce() + Send + 'static>`. Boxing erases the closure's concrete type so different closures fit in one channel. It must be `Send` to cross threads and `'static` because workers outlive the call.
- `execute` boxes the closure and sends it.
- A channel has **one** receiver, but all workers need to pull from it. Share it: `Arc<Mutex<Receiver<Job>>>`. A worker locks, calls `recv()`, and the lock is released at the end of that statement, so other workers can take their turn while this one runs its job.

```rust
let job = receiver.lock().unwrap().recv();   // guard is a temporary: dropped at the semicolon
match job {
    Ok(job) => job(),
    Err(_) => break,                          // channel closed: time to exit
}
```

Careful: `while let Ok(job) = receiver.lock().unwrap().recv() { job() }` keeps the guard alive for the whole loop body, so only one job could run at a time. Binding `job` in a separate `let` statement avoids that.

## Shutting down in Drop

When the pool is dropped there is no more work coming. Closing the channel makes every `recv()` return `Err`, which is each worker's signal to leave its loop. To close it, **drop the sender** first, then join the workers so no job is abandoned:

```rust
impl Drop for ThreadPool {
    fn drop(&mut self) {
        drop(self.sender.take());           // needs Option<Sender<Job>>
        for w in self.workers.drain(..) {
            w.join().unwrap();
        }
    }
}
```

`Option::take` lets you move the sender out of `&mut self`. Dropping the pool thus *waits for all queued jobs to finish*, which makes the pool easy to test: once `drop(pool)` returns, all work is done.

## This lesson's exercise

The starter pool "works" by running jobs inline on the caller's thread. Replace that with real workers, a shared receiver, and a clean `Drop`.
