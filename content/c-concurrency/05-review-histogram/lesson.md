---
title: "Review: Parallel Histogram"
summary: A consolidation exercise. Count byte values with several threads, each counting its own slice and merging into the shared result under a mutex.
minutes: 20
difficulty: medium
kind: review
tags: threads, mutex
id: c-concurrency/review-histogram
mode: harness
---
This is a **review lesson**. It introduces nothing new. You have started and joined threads with their own arguments ([Starting and Joining Threads](/learn/c-concurrency/threads-create-join)), split an array into slices ([Splitting Work and Returning Results](/learn/c-concurrency/return-values)) and protected shared data with a mutex ([Data Races and Mutexes](/learn/c-concurrency/mutex)). Here you use all three together.

A **histogram** counts how many times each value appears. For bytes there are 256 possible values, so it is an array of 256 counters.

The fast way to build one in parallel is a pattern you will meet again and again:

1. Each thread counts its own slice into a **private** histogram. No sharing, no locking, no waiting.
2. When a thread has finished counting, it takes the mutex **once** and adds its private counts into the shared histogram.

Locking once per thread instead of once per byte is the whole trick. Compare the cost: 256 additions under one lock, against a lock for every one of a million bytes.

```c
typedef struct {
    const unsigned char *data;
    size_t lo, hi;          // this thread's slice
    long *shared;           // the final 256 counters
    pthread_mutex_t *lock;  // guards shared
} Task;
```

Write the worker and the function that splits the data between threads.
