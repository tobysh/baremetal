---
title: Ring Buffer Queue
summary: Build a fixed-size FIFO queue on an array that wraps around. Learn head and length bookkeeping, modular indexing, and how stacks and queues differ.
minutes: 30
difficulty: medium
tags: queues, arrays, data-structures
id: c-data-structures/ring-buffer-queue
mode: harness
---
A **stack** is last-in, first-out: a pile of plates. A **queue** is first-in, first-out: a line at a shop. A stack is easy on an array because you only touch one end. A queue touches both ends, and that is the interesting part.

## The naive way is slow

Store items in an array, add at the back, and remove from the front by shifting everything left. Each removal then costs O(n). Alternatively, never shift and just move a front index forward, but then the front creeps to the end of the array and the space before it is wasted.

## Wrap around

A **ring buffer** (circular buffer) fixes this by treating the array as a circle. When an index reaches the end, it wraps back to 0 using the remainder operator:

```c
typedef struct {
    int *buf;
    int cap;    // size of the array
    int head;   // index of the oldest item
    int len;    // how many items are stored
} Queue;

// push: write at the first free slot after the last item
int tail = (q->head + q->len) % q->cap;
q->buf[tail] = x;
q->len++;

// pop: take the oldest item and move head forward
int x = q->buf[q->head];
q->head = (q->head + 1) % q->cap;
q->len--;
```

Storing `len` rather than a `tail` index removes a classic puzzle: when `head == tail`, is the queue empty or full? With `len` you simply compare `len == 0` and `len == cap`.

Neither operation moves any data, so both are O(1). Ring buffers like this are everywhere: keyboard input, network packets, audio streams, log tails.

## Common mistakes

- Using `%` on a negative number (it can give a negative result in C). Only add forwards.
- Forgetting to check full/empty before writing/reading, which overwrites live data or returns garbage.
- Leaking `buf` because nothing frees it.

Implement the queue.
