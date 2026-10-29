---
title: Slices, Sorting and Searching
summary: Sort with keys and comparators, binary search, dedup, retain and destructure slices with patterns.
minutes: 25
difficulty: medium
tags: slices, sorting, binary-search
id: rust-bridge/slices-and-sorting
mode: harness
---
A slice `&[T]` (or `&mut [T]`) is a pointer plus a length: a view into part of an array or `Vec`. Functions that take `&[T]` accept arrays, vectors and sub-ranges alike, which is why you should prefer it over `&Vec<T>`.

## Sorting

`v.sort()` sorts in place using the type's ordering. For anything else:

```rust
v.sort_by_key(|p| p.age);                          // by a key
v.sort_by(|a, b| b.score.cmp(&a.score));           // custom comparator, descending
v.sort_by(|a, b| a.partial_cmp(b).unwrap());       // floats are not Ord
v.sort_by_key(|p| std::cmp::Reverse(p.age));       // descending by key
```

All of these are **stable**: items that compare equal keep their relative order. `sort_unstable*` is a little faster and does not promise that.

## Searching a sorted slice

`binary_search(&x)` returns `Ok(index)` if found, or `Err(index)` where `x` could be inserted to keep the order:

```rust
let v = [1, 3, 5];
v.binary_search(&3); // Ok(1)
v.binary_search(&4); // Err(2)
```

It only works if the slice is already sorted.

## Cleaning a Vec

- `v.dedup()` removes **consecutive** duplicates (sort first to remove all).
- `v.retain(|x| *x > 0)` keeps only matching items.
- `v.insert(i, x)`, `v.remove(i)`, `v.swap(i, j)`, `v.rotate_left(k)`, `v.split_at(mid)`.

## Slice patterns

You can match on the shape of a slice:

```rust
match v {
    [] => "empty",
    [x] => "one",
    [first, .., last] => "many",
}
```

## This lesson's exercise

Five functions that combine these tools.
