---
title: flat_map, Peekable and partition
summary: Flatten nested data, look one item ahead with peekable, split a stream in two with partition and stop early with find_map.
minutes: 20
difficulty: medium
tags: iterators, peekable, flat-map
id: rust-bridge/flat-map-and-peekable
mode: harness
---
Some jobs do not fit a plain `map` and `filter`. Four more tools cover most of them.

## flat_map and flatten

`map` turns one item into one item. `flat_map` turns one item into **many** and joins the results:

```rust
let lines = ["a b", "c"];
let words: Vec<&str> = lines.iter().flat_map(|l| l.split_whitespace()).collect(); // ["a","b","c"]
```

`flatten` joins things that are already iterable. Since `Option` is iterable (zero or one item), `flatten` on an iterator of options drops the `None`s and unwraps the `Some`s.

## find_map and partition

`find_map(f)` runs `f` on each item and stops at the first `Some`. It is `find` and `map` in one step.

`partition(pred)` splits a stream into two collections, the items that satisfy the predicate and the rest:

```rust
let (even, odd): (Vec<i32>, Vec<i32>) = (1..=6).partition(|n| n % 2 == 0);
```

## Peekable

An iterator normally gives each item away for good. `.peekable()` adds `peek()`, which shows the next item without consuming it. That is exactly what you need to group runs of equal values:

```rust
let mut it = "aab".chars().peekable();
while let Some(c) = it.next() {
    while it.peek() == Some(&c) { it.next(); } // swallow repeats
}
```

## This lesson's exercise

Practise all four on text and numbers. The tests use many different inputs.
