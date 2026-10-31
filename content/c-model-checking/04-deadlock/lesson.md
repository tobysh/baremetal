---
title: Find the Deadlock
summary: Report a state where unfinished threads wait on each other and no thread can move, with the schedule that reaches it.
minutes: 35
difficulty: medium
tags: model-checking, deadlock, lock-ordering
id: c-model-checking/deadlock
mode: harness
---
You met lock ordering in `c-concurrency`. A model checker finds the problem without waiting for it to happen on real hardware.

## Definition

A **deadlock** is a reachable state in which some thread has not finished and **no** thread is enabled. In our model that is exactly: not `all_done`, and `thread_enabled` is false for every thread.

Two cases the tests cover:

- **Opposite lock order.** Thread 0 takes lock 0 then lock 1; thread 1 takes lock 1 then lock 0. The schedule `0 1` leaves each holding what the other wants.
- **A leaked lock.** A thread that finishes without unlocking leaves its lock held forever. Another thread waiting for it is stuck even though nobody is "waiting on each other". The model treats this the same way.

Using a global lock order (always lock 0 before lock 2, say) removes the cycle, and the checker confirms it by exhausting every schedule.

## Checkpoint

Your checker from lesson 3 must still find lost updates; the new check must not break it.

A model deadlock is only as real as the model: here locks are not re-entrant and have no timeouts, and a thread blocked in `LOCK` is stuck for good.
