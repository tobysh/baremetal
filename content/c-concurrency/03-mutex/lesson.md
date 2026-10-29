---
title: Data Races and Mutexes
summary: See why two threads incrementing one counter lose updates, and fix it with a mutex that covers the whole read-modify-write.
minutes: 25
difficulty: medium
tags: threads, mutex, data-race
id: c-concurrency/mutex
mode: harness
---
Threads share memory, and that is where the trouble starts. Look at this innocent line:

```c
counter++;
```

It is really **three** steps: load `counter` into a register, add one, store it back. If two threads do this at the same moment, both can load `5`, both add one, and both store `6`. One increment is lost. This is a **data race**: two threads touch the same memory, at least one writes, and nothing orders them. In C the result is undefined behaviour, and in practice the final total is *wrong and different each run*.

## Mutual exclusion

A **mutex** lets only one thread at a time into a piece of code, called the **critical section**.

```c
pthread_mutex_t lock;
pthread_mutex_init(&lock, NULL);

pthread_mutex_lock(&lock);
counter++;                      // only one thread is ever here
pthread_mutex_unlock(&lock);

pthread_mutex_destroy(&lock);
```

Keep the lock and the data it guards together in one struct, so it is obvious what protects what:

```c
typedef struct {
    pthread_mutex_t lock;
    long value;
} Counter;
```

## Lock around the whole decision

A lock must cover the **check and the action together**. This is wrong even though each line looks safe:

```c
if (c->value < limit) {      // thread A checks: 99 < 100, ok
                             // thread B checks: 99 < 100, ok
    c->value++;              // both add: 101
}
```

The check and the update have to sit inside one lock/unlock pair.

## Habits

- Keep critical sections **short**. Never call something slow while holding a lock.
- Always unlock on **every** path out of the function (early `return` included).
- Locking is not free, but a correct slow program beats a fast wrong one.

## Your turn

Fill in the counter functions, then the function that hammers one counter from many threads. The tests run many threads against your code, and a missing lock shows up as a wrong total.
