---
title: Parallel Map-Reduce
summary: Write a generic parallel map-reduce with scoped threads, reason about Send, Sync and associativity, and use it to count words.
minutes: 40
difficulty: hard
tags: threads, scope, generics, map-reduce
id: rust-concurrency/parallel-map-reduce
mode: harness
---
**Map-reduce** is the shape behind a great deal of parallel code:

1. **Map**: turn each input item into a partial result, independently.
2. **Reduce**: combine partial results into one with a binary operation.

Because the map step does not share anything, it parallelises beautifully. Split the input into one chunk per worker; each worker maps and reduces *its own chunk* into a single value; finally the main thread reduces the handful of per-worker values.

```text
items:   [ ........ | ........ | ........ ]
           worker 0    worker 1    worker 2      (map + reduce locally)
partials:     p0          p1          p2
result:   reduce(reduce(identity, p0), p1), p2)    (in chunk order)
```

## The signature

```rust
fn map_reduce<T, U, M, R>(items: &[T], workers: usize, map: M, identity: U, reduce: R) -> U
where
    T: Sync,                 // &T crosses threads, so T must be Sync
    U: Send + Clone,         // partial results are sent back; each worker starts from a clone of identity
    M: Fn(&T) -> U + Sync,   // shared by reference between threads, so Sync (and Fn, not FnMut)
    R: Fn(U, U) -> U + Sync,
```

Look at how the bounds trace back to the previous lessons: `Sync` where something is *borrowed* from several threads, `Send` where something is *moved* to or from one. With `thread::scope` there is no `'static` bound, so closures may borrow `map` and `reduce` directly and the caller can pass closures that borrow local variables.

## Why associativity matters

Chunks are reduced separately and then combined, so `reduce` must be **associative**: `(a ⊕ b) ⊕ c == a ⊕ (b ⊕ c)`. Addition, `max` and string concatenation qualify. Subtraction does not. It need not be *commutative* as long as you combine partials **in chunk order**, so concatenation gives the same result as a sequential loop. `identity` must satisfy `reduce(identity, x) == x`.

## Splitting

Chunk size is `ceil(len / workers)`, at least 1; use `items.chunks(size)`. An empty slice has no chunks and therefore returns `identity`. Treat `workers == 0` as 1.

## Using it

A word count is a map-reduce where `map` turns one line into a `BTreeMap<String, usize>` and `reduce` merges two maps:

```rust
let counts = map_reduce(&lines, 4, |line| count_words(line), BTreeMap::new(), merge);
```

## This lesson's exercise

Write `map_reduce` itself, then `word_frequencies` on top of it.
