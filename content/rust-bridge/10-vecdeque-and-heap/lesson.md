---
title: VecDeque and BinaryHeap
summary: Use a double-ended queue for FIFO work and a priority queue for always-take-the-best, with Reverse for a min-heap.
minutes: 20
difficulty: medium
tags: collections, vecdeque, binaryheap, queues
id: rust-bridge/vecdeque-and-heap
mode: harness
---
`Vec` is fast at the back but slow at the front: `remove(0)` shifts every element. Two other collections fit queue-shaped problems.

## VecDeque: a ring buffer

`VecDeque<T>` is a growable circular buffer: constant-time push and pop at **both** ends.

```rust
use std::collections::VecDeque;

let mut q = VecDeque::new();
q.push_back(1);
q.push_back(2);
q.push_front(0);        // [0, 1, 2]
q.pop_front();          // Some(0), first in first out
q.rotate_left(1);       // move the front item to the back
```

Reach for it for queues, breadth-first search, sliding windows and round-robin scheduling. It indexes like a Vec (`q[i]`) and iterates front to back.

## BinaryHeap: a priority queue

`BinaryHeap<T>` always hands you the **largest** item first (`pop`, `peek`), in `O(log n)` per operation. Items must be `Ord`.

```rust
use std::collections::BinaryHeap;
use std::cmp::Reverse;

let mut h = BinaryHeap::new();
h.push(3); h.push(9); h.push(5);
h.pop();                 // Some(9)

let mut small = BinaryHeap::new();
small.push(Reverse(3));  // Reverse flips the order: a min-heap
small.push(Reverse(1));
small.pop();             // Some(Reverse(1))
```

To prioritise records, put the priority first in a tuple, `(priority, name)`: tuples compare element by element. Remember that iterating a heap does **not** give sorted order; only `pop` does (or `into_sorted_vec`).

## This lesson's exercise

A round-robin scheduler, a stream's k smallest values, a task scheduler and a sliding window.
