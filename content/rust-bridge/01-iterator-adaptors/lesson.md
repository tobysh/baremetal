---
title: Iterator Adaptors in Depth
summary: Chain zip, enumerate, windows, chunks, scan and fold to replace index loops with short, lazy pipelines.
minutes: 20
difficulty: easy
tags: iterators, slices, adaptors
id: rust-bridge/iterator-adaptors
mode: harness
---
In C you walk an array with `for (i = 0; i < n; i++)` and index by hand. Rust prefers iterator **adaptors**: small methods that each reshape a stream of values. They are lazy, so nothing runs until a consumer such as `collect`, `sum` or `fold` pulls the values through.

## The everyday ones

```rust
let v = vec![3, 1, 4, 1, 5];

for (i, x) in v.iter().enumerate() { /* i = 0.., x = &3, &1, ... */ }

let dot: i32 = v.iter().zip(&v).map(|(a, b)| a * b).sum(); // pairs up two streams
let big: Vec<usize> = v.iter().enumerate()
    .filter(|(_, &x)| x > 2)
    .map(|(i, _)| i)
    .collect();                                           // [0, 2, 4]
```

`zip` stops at the shorter side. `enumerate` pairs each item with its index.

## Looking at neighbours: windows and chunks

Slices have two methods that give you groups, not single items:

- `v.windows(2)` yields overlapping pairs: `[3,1]`, `[1,4]`, `[4,1]`, `[1,5]`. A slice shorter than 2 yields nothing.
- `v.chunks(2)` yields non-overlapping pieces: `[3,1]`, `[4,1]`, `[5]`. The last one may be shorter.

## Carrying state: fold and scan

`fold(start, |acc, x| ...)` reduces everything to one value. `scan(start, |state, x| ...)` is the lazy cousin that yields something at every step, so it gives you running totals:

```rust
let running: Vec<i32> = v.iter()
    .scan(0, |sum, &x| { *sum += x; Some(*sum) })
    .collect(); // [3, 4, 8, 9, 14]
```

## This lesson's exercise

Five small functions over slices, each a short pipeline with no index arithmetic.
