---
title: A Lock-Free Ring Buffer
summary: Pass numbers from one producer thread to one consumer thread through a ring buffer using only atomics and release/acquire ordering.
minutes: 40
difficulty: hard
tags: threads, atomics, lock-free, queue
id: c-concurrency/lock-free-ring
mode: harness
---
You have built a bounded buffer with a mutex and condition variables. For the special case of **one producer and one consumer**, you can drop the lock entirely. This is a single-producer, single-consumer (SPSC) queue, and it is used for audio, networking and logging, where a thread must never block on a lock.

## The idea

A fixed array `buf` of `RING_SIZE` slots, and two counters that only ever go up:

- `tail`: how many items the producer has pushed in total. **Only the producer writes it.**
- `head`: how many items the consumer has popped in total. **Only the consumer writes it.**

The number of items stored is `tail - head`. The slot for item number `n` is `buf[n % RING_SIZE]`. The counters never wrap around in practice, and unsigned subtraction stays right even if they did.

The queue is **empty** when `head == tail` and **full** when `tail - head == RING_SIZE`.

Because each counter has exactly one writer, no two threads ever write the same variable, so no lock is needed. The only job left is **ordering**.

## Push

```c
size_t t = atomic_load_explicit(&r->tail, memory_order_relaxed);   // my own counter
size_t h = atomic_load_explicit(&r->head, memory_order_acquire);   // the other side's
if (t - h == RING_SIZE) return 0;                                  // full
r->buf[t % RING_SIZE] = value;                                     // write the slot
atomic_store_explicit(&r->tail, t + 1, memory_order_release);      // publish it
return 1;
```

- The producer reads its **own** `tail` relaxed: nobody else changes it.
- It reads `head` with **acquire**, so that the consumer's earlier read of a slot is finished before the producer overwrites that slot.
- It publishes the new item with a **release** store of `tail`, so the slot write cannot be seen after the counter change. This is the mailbox pattern from the last lesson.

## Pop

Pop is the mirror image: read your own `head` relaxed, read `tail` with acquire (if equal, the queue is empty), copy the slot out, and release-store `head + 1`.

## Your turn

Implement `ring_push` and `ring_pop`, and the test will run a producer thread and a consumer thread against each other, sending 100000 numbers that must arrive in order, with none lost or repeated. Both functions must return immediately: `0` means "full" or "empty", and the caller decides whether to retry.
