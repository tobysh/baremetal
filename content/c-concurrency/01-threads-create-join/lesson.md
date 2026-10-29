---
title: Starting and Joining Threads
summary: Start threads with pthread_create, give each one its own argument, and wait for them with pthread_join.
minutes: 20
difficulty: easy
tags: threads, pthreads
id: c-concurrency/threads-create-join
mode: harness
---
Until now your program did one thing at a time. A **thread** is another line of execution inside the same program. All threads share the same memory, and the operating system runs them on different cores at the same time (or takes turns on one core).

C's standard thread API on Linux is **pthreads**. Add `#include <pthread.h>` and you get two functions that matter most:

```c
pthread_t t;
int err = pthread_create(&t, NULL, worker, &arg);   // start a thread
pthread_join(t, NULL);                               // wait until it finishes
```

The thread runs a function with exactly this shape:

```c
void *worker(void *arg) {
    // arg is whatever you passed to pthread_create
    return NULL;
}
```

`void *` means "a pointer to anything", so you cast it back to the real type inside:

```c
typedef struct { int input; int output; } Job;

void *worker(void *arg) {
    Job *job = arg;           // no cast needed in C
    job->output = job->input + 1;
    return NULL;
}
```

## Rules to remember

- `pthread_create` returns right away. The new thread runs **at the same time** as the code after it.
- `pthread_join` blocks until that thread has finished. After it returns, everything the thread wrote is safe to read.
- The argument must **stay alive** while the thread uses it. Never pass the address of a local variable that goes out of scope before the thread ends. Here, each thread gets its own `Job` in an array that outlives the joins.
- Give each thread its **own** data. Two threads writing the same variable is a data race (the next lessons are about that).

## Your turn

Write the worker that squares a number, then start one thread per element, and join them all.
