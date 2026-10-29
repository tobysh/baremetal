---
title: Splitting Work and Returning Results
summary: Split an array between threads, let each thread return its result as the thread function's return value, and collect it with pthread_join.
minutes: 25
difficulty: easy
tags: threads, pthreads
id: c-concurrency/return-values
mode: harness
---
In [the last lesson](/learn/c-concurrency/threads-create-join) each thread wrote its answer into a struct we gave it. There is a second way: the thread function **returns** a `void *`, and `pthread_join` hands it back.

```c
void *worker(void *arg) {
    long result = 42;
    return (void *)(intptr_t)result;      // goes to pthread_join
}

void *ret;
pthread_join(t, &ret);                    // ret holds what the thread returned
long answer = (long)(intptr_t)ret;
```

`void *` is just a pointer-sized box. A small number such as a `long` fits inside it, and `intptr_t` (from `<stdint.h>`) is the integer type that is exactly as wide as a pointer, so the casts are lossless. It is a common trick for returning one number.

## Never return a pointer to a local

A thread's local variables vanish when it returns. Returning `&result` hands the caller a dangling pointer, which is undefined behaviour. For bigger results, return a pointer to heap memory (`malloc`) and let whoever joins free it, or write into a struct the caller owns, as in the last lesson. The same rule you learned for ordinary functions holds here.

## Splitting an array

The classic use of threads is to cut a big job into slices. Each thread gets a different slice, so no two threads touch the same data:

```c
typedef struct { const int *a; int lo, hi; } Range;   // sum a[lo..hi)
```

With `n` items and `k` threads, give each thread `chunk = (n + k - 1) / k` items (round up). Thread `i` takes `[i*chunk, min((i+1)*chunk, n))`. When `n` is smaller than `k`, the last threads get an empty range, which is fine.

Because every thread only **reads** the shared array and **writes** only its own result, there is nothing to lock. Sharing read-only data is always safe.

## Your turn

Write a worker that sums a range and returns the total, then use it to add up an array with several threads.
