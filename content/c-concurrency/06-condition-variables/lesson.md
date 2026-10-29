---
title: Condition Variables and a Bounded Buffer
summary: Make threads wait for a condition without spinning, and use that to build the classic bounded buffer between producers and consumers.
minutes: 35
difficulty: hard
tags: threads, condition-variables, queue
id: c-concurrency/condition-variables
mode: harness
---
A mutex answers "who may touch this data?". It does not answer "**wait until** the data is ready". Suppose consumers take items from a queue that producers fill. What should a consumer do when the queue is empty?

Spinning (`while (empty) {}`) burns a whole core. Locking and checking in a loop is no better. What you want is to **sleep** until another thread says "something changed". That is a **condition variable**.

```c
pthread_cond_t ready;
pthread_cond_init(&ready, NULL);

// the waiting side
pthread_mutex_lock(&lock);
while (queue_is_empty)                 // always a while, never an if
    pthread_cond_wait(&ready, &lock);  // unlocks, sleeps, re-locks on wake
take_an_item();
pthread_mutex_unlock(&lock);

// the signalling side
pthread_mutex_lock(&lock);
add_an_item();
pthread_cond_signal(&ready);           // wake one waiter
pthread_mutex_unlock(&lock);
```

## The rules

- A condition variable always works **together with a mutex** that guards the condition. `pthread_cond_wait` atomically unlocks the mutex and goes to sleep, then locks it again before it returns.
- Wait in a **`while` loop**, not an `if`. A thread can wake up when the condition is already false again, because another thread got there first, or because of a *spurious wakeup* (the system is allowed to wake you for no reason). Re-check, and sleep again if needed.
- Change the condition **while holding the lock**, then signal. `pthread_cond_signal` wakes one waiter, `pthread_cond_broadcast` wakes all of them.

## A bounded buffer

A **bounded buffer** is a fixed-size queue between producers and consumers. It needs two conditions: consumers wait while it is **empty**, producers wait while it is **full**. That gives two condition variables on one mutex: `not_empty` and `not_full`.

Producers also need a way to say "no more items are coming", or consumers would wait forever. The buffer has a `closed` flag. `bb_close` sets it and *broadcasts* to wake everyone. A consumer on a closed buffer still drains what is left, and only when it is empty and closed does `bb_get` report "done".

## Your turn

The struct and `bb_init` are written for you. Implement put, get and close. Many producers and consumers hammer your buffer in the tests, and every item must arrive exactly once, in order per producer.
