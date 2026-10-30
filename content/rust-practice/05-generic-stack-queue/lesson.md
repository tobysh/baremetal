---
title: A Generic Stack and Queue
summary: Build Stack<T> and Queue<T> on top of Vec, practise Option returns and borrowed peeks, and use a stack to check balanced brackets.
minutes: 20
difficulty: medium
tags: generics, data-structures
id: rust-practice/generic-stack-queue
mode: harness
---
Two of the most useful containers are also two of the simplest. A **stack** is last in, first out, like a pile of plates. A **queue** is first in, first out, like people waiting for a well. Both are easy to build as generic types, so one definition holds anything: numbers, strings, your own structs.

```rust
struct Stack<T> {
    items: Vec<T>,
}

impl<T> Stack<T> {
    fn new() -> Stack<T> {
        Stack { items: Vec::new() }
    }

    fn push(&mut self, item: T) {
        self.items.push(item);
    }

    fn pop(&mut self) -> Option<T> {
        self.items.pop()
    }
}
```

## Return Option, not a panic

Popping an empty stack is not a bug in your program; it just means there is nothing there. So `pop` returns `Option<T>`. The same goes for `peek`, which looks at the top item **without removing it**. Because the item stays inside the stack, `peek` returns a reference: `Option<&T>`. The borrow checker then makes sure you cannot push or pop while still holding that reference.

## A queue from two stacks

`Vec::remove(0)` shifts everything along, which is slow for big queues. A classic trick avoids it: keep two vectors. New items go onto `inbox`. To take one out, look in `outbox`; if that is empty, pour the whole inbox into it in reverse. Reversing turns "last in" into "first out", so items now come off the end of `outbox` in arrival order. Each item moves at most once, so the average cost is constant.

## This lesson's exercise

Write `Stack<T>` and `Queue<T>`, then use a `Stack<char>` to decide whether the brackets in a string are balanced.
