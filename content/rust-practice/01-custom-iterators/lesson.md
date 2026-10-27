---
title: Writing Your Own Iterator
summary: Implement the Iterator trait for your own type and unlock map, filter, take_while, sum and the rest of the adaptors for free.
minutes: 15
difficulty: easy
tags: iterators, traits
id: rust-practice/custom-iterators
mode: harness
---
You have used iterators since your first `for` loop. Underneath, an iterator is any type that implements one small trait:

```rust
trait Iterator {
    type Item;
    fn next(&mut self) -> Option<Self::Item>;
}
```

`next` hands back `Some(value)` while there is more to give, and `None` once it is finished. That is the whole contract. Write `next`, and your type gets dozens of methods without any extra effort.

## A counter

```rust
struct Up { n: u32 }

impl Iterator for Up {
    type Item = u32;

    fn next(&mut self) -> Option<u32> {
        self.n += 1;
        Some(self.n)
    }
}
```

`type Item = u32;` names what the iterator produces. The struct itself holds the state: each call to `next` updates `self` and returns the following value. This one never ends, which is fine, because adaptors can cut it short:

```rust
let firsts: Vec<u32> = Up { n: 0 }.take(3).collect(); // [1, 2, 3]
```

## Adaptors are lazy

`map`, `filter`, `take_while` and friends do not do any work when you call them. They wrap your iterator in a new one. Nothing happens until a **consumer** such as `collect`, `sum`, `count` or a `for` loop asks for values, one at a time. That is why an endless iterator is safe to chain, as long as something stops it.

```rust
let total: u32 = Up { n: 0 }
    .filter(|n| n % 2 == 0)
    .take_while(|n| *n < 10)
    .sum(); // 2 + 4 + 6 + 8 = 20
```

## This lesson's exercise

Write two iterators, `Fib` (the Fibonacci numbers, starting 0, 1, 1, 2, 3, ...) and `Countdown` (a number down to 1, then it stops). Then use adaptors on `Fib` to add up the even Fibonacci numbers below a limit.
