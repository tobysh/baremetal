---
title: Bounds, where Clauses and Iterator
summary: Write generic functions with trait bounds and where clauses, implement the Iterator trait for your own struct, and accept anything iterable.
minutes: 30
difficulty: medium
tags: generics, traits, iterators
id: rust-intermediate/generic-bounds-iterators
mode: harness
---
A generic function states what it needs from its type parameters. Those requirements are **bounds**:

```rust
fn show_all<T: Display>(items: &[T]) { /* ... */ }
```

When the bounds get long, move them into a `where` clause. It means exactly the same:

```rust
fn largest_by_key<T, K, F>(items: &[T], key: F) -> Option<&T>
where
    K: PartialOrd,
    F: Fn(&T) -> K,
{ /* ... */ }
```

Here `F` is any closure (or function) that turns a `&T` into some comparable `K`. The function can compare any items by any key without knowing either type.

## Implementing Iterator

`Iterator` has one required method, `next`, plus one associated type, `Item`. Everything else (`map`, `filter`, `sum`, `collect`) is a default method:

```rust
struct Counter { n: u32 }

impl Iterator for Counter {
    type Item = u32;
    fn next(&mut self) -> Option<u32> {
        if self.n == 0 { None } else { self.n -= 1; Some(self.n) }
    }
}
```

`next` returns `Some(value)` until the iterator is exhausted and `None` afterwards.

## Accepting anything iterable

`IntoIterator` is what a `for` loop calls. A function that takes `I: IntoIterator` accepts a `Vec`, an array, a range or your own iterator. Bounds can talk about the associated type: `I::Item: Display` says the *items* must be printable.

```rust
fn first_two<I: Iterator>(it: I) -> Vec<I::Item> { it.take(2).collect() }
```

## This lesson's exercise

Implement `Countdown` (an iterator), `largest_by_key`, `summarize` and `every_other`.
