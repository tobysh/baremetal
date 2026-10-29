---
title: A Thread Pool
summary: Build a pool of worker threads that pull tasks from a shared queue, with a wait for all work to finish and a clean shutdown.
minutes: 45
difficulty: hard
tags: threads, condition-variables, queue
id: c-concurrency/thread-pool
mode: harness
---
Starting a thread costs time and memory. A server that makes a new thread for every request wastes both, and can be knocked over by a flood of requests. A **thread pool** starts a fixed number of workers once. Work is then handed over as small **tasks**, and idle workers pick them up.

```
submit(task) ──▶ [ queue of tasks ] ──▶ worker 1
                                    ──▶ worker 2
                                    ──▶ worker 3
```

This lesson puts together nearly everything in the course: a mutex guarding a queue, condition variables for "there is work" and "everything is finished", and careful shutdown.

## Parts

A task is a function and an argument, in a linked list:

```c
typedef void (*TaskFn)(void *);
typedef struct Task { TaskFn fn; void *arg; struct Task *next; } Task;
```

The pool keeps `head` and `tail` pointers (the queue), a `pending` count of tasks that are **queued or running**, and a `shutdown` flag. Two condition variables:

- `has_work`: workers wait on it while the queue is empty.
- `all_done`: `pool_wait` waits on it until `pending` reaches 0.

## The worker loop

```c
for (;;) {
    lock
    while (queue empty && !shutdown) wait(has_work)
    if (queue empty) { unlock; return }       // shutting down and nothing left
    take the first task off the queue
    unlock
    run it               // outside the lock! otherwise only one task runs at a time
    free it
    lock
    pending--; if (pending == 0) broadcast(all_done)
    unlock
}
```

Notice two details. The task runs **without the lock**, otherwise the pool would be sequential. And `pending` drops only *after* the task has finished, so `pool_wait` really waits for running tasks, not just queued ones.

## Shutdown

`pool_destroy` sets `shutdown`, wakes every worker with a broadcast, and joins them. Workers keep taking tasks while the queue is not empty and leave only when it is, so nothing already submitted is lost.

## Your turn

The structs and `pool_create`'s skeleton are in the starter. Fill in the four functions and the worker. The tests run thousands of tasks, check that tasks really run on several worker threads at once, and check shutdown. A watchdog reports a hang instead of waiting forever.
