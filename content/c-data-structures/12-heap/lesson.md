---
title: Binary Heap and Priority Queue
summary: Store a tree in a plain array and keep the smallest item on top. Write sift-up and sift-down, a min-heap priority queue, and heap sort.
minutes: 40
difficulty: medium
tags: heap, trees, priority-queue, data-structures
id: c-data-structures/heap
mode: harness
---
A **priority queue** hands you the most urgent item first, whatever order items arrived in: the next job to run, the nearest unvisited city, the lowest-cost path. A **binary heap** implements it with fast insert and fast "remove the smallest".

## A tree in an array

A heap is a complete binary tree stored level by level in an array, with no pointers at all. For the element at index `i`:

- parent: `(i - 1) / 2`
- left child: `2 * i + 1`
- right child: `2 * i + 2`

The **min-heap property**: every element is `<=` its children. So the smallest element is always at index 0. Nothing else is promised: siblings have no order.

## Push: sift up

Put the new item at the end (the next free slot), then swap it with its parent while it is smaller than that parent.

```c
size_t i = h->len++;
h->a[i] = x;
while (i > 0 && h->a[(i - 1) / 2] > h->a[i]) {
    swap(&h->a[(i - 1) / 2], &h->a[i]);
    i = (i - 1) / 2;
}
```

## Pop: sift down

The minimum is `a[0]`. Remove it by moving the **last** element to index 0, shrinking the length, and then swapping it down with the *smaller* child until neither child is smaller.

```c
while (1) {
    size_t l = 2 * i + 1, r = l + 1, smallest = i;
    if (l < len && a[l] < a[smallest]) smallest = l;
    if (r < len && a[r] < a[smallest]) smallest = r;
    if (smallest == i) break;
    swap(&a[i], &a[smallest]);
    i = smallest;
}
```

Both operations walk one root-to-leaf path, so they cost O(log n).

## Heap sort

Push all `n` items into a heap, then pop them one by one: they come out in ascending order. That is O(n log n).

## Common mistakes

- Swapping with the larger child, or with the left child without comparing.
- Checking `r < len` after using `a[r]`.
- Forgetting that after a pop with `len == 1` there is nothing to sift.
- Using `(i - 1) / 2` with `i == 0` (it is 0 with unsigned maths only by luck; guard with `i > 0`).

Implement the heap.
