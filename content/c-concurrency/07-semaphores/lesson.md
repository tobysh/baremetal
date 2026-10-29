---
title: Semaphores
summary: Use counting semaphores to hand turns between threads in strict order, and to limit how many threads use something at once.
minutes: 25
difficulty: medium
tags: threads, semaphores
id: c-concurrency/semaphores
mode: harness
---
A **semaphore** is a counter that threads wait on. Include `<semaphore.h>`:

```c
sem_t s;
sem_init(&s, 0, 3);     // 0 = shared between threads of this process, starts at 3

sem_wait(&s);           // if the count is > 0, subtract 1 and go on; if 0, sleep until it isn't
// ... use the resource ...
sem_post(&s);           // add 1 and wake a waiter

sem_destroy(&s);
```

`sem_wait` is also called "P" or "down", `sem_post` is "V" or "up". Unlike a mutex, any thread may post a semaphore, not just the one that waited. That makes semaphores good at two jobs.

## Job 1: limiting access

A semaphore starting at `N` lets at most `N` threads through at once. With `N = 1` it works like a mutex. With `N = 3` it is a gate for "at most three database connections", "at most three downloads at a time", and so on.

```c
sem_wait(&slots);     // take a slot (waits if all are taken)
use_the_resource();
sem_post(&slots);     // give it back
```

## Job 2: signalling and taking turns

A semaphore starting at **0** means "nothing to do yet". A thread waits on it, and another thread posts it when there is something. With two semaphores two threads can alternate perfectly, like a game of ping-pong. Each thread waits on its own semaphore, does its move, and posts the other's:

```
ping:  wait(ping_turn)  ...  post(pong_turn)
pong:  wait(pong_turn)  ...  post(ping_turn)
```

`ping_turn` starts at 1 so ping goes first, and `pong_turn` starts at 0.

## Your turn

Do both jobs. First write the two functions of a `Limiter`, which lets at most `limit` threads be inside at once. Then make two threads play ping-pong and record each move, which must come out in strict alternation. The tests measure the real number of threads inside your limiter.
