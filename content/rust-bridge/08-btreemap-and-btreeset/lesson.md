---
title: BTreeMap and BTreeSet
summary: Keep keys sorted, walk them in order and query ranges, first and last entries with the ordered collections.
minutes: 20
difficulty: medium
tags: collections, btreemap, btreeset, ranges
id: rust-bridge/btreemap-and-btreeset
mode: harness
---
`HashMap` is fastest for plain lookups but iterates in an unpredictable order. `BTreeMap` and `BTreeSet` keep their keys **sorted** (they are balanced trees with many keys per node, which is cache friendly). Lookups cost `O(log n)` instead of `O(1)`, and in return you get order.

```rust
use std::collections::BTreeMap;

let mut m = BTreeMap::new();
m.insert("pear", 3);
m.insert("apple", 5);
for (k, v) in &m { println!("{k} {v}"); }   // apple first, then pear
```

The entry API works exactly as for `HashMap`: `*m.entry(k).or_insert(0) += 1`.

## Things only an ordered map can do

```rust
m.first_key_value()         // Option<(&K, &V)>, the smallest key
m.last_key_value()          // the largest
m.range(10..20)             // iterator over keys 10 <= k < 20
m.range(..=5)               // keys up to and including 5
m.range(7..).next()         // the first key >= 7  (a "ceiling" lookup)
m.range(..7).next_back()    // the last key < 7    (a "floor" lookup)
m.pop_first()               // remove and return the smallest
```

`BTreeSet` is the same idea without values: sorted, unique, with `range`, `first()`, `last()` and the set operations.

Keys must implement `Ord` (a total order). Floats do not, so you cannot use `f64` as a key directly.

## This lesson's exercise

A word index, a price table with range queries, and a sorted set of intervals.
