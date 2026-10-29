---
title: HashSet and the Entry API
summary: Use HashSet for membership and set algebra, and the entry API to count, group and update HashMap values in one lookup.
minutes: 20
difficulty: medium
tags: collections, hashset, hashmap, entry
id: rust-bridge/hashset-and-entry
mode: harness
---
You have met `HashMap` already. Two more tools make it far more pleasant.

## HashSet: membership without values

`HashSet<T>` stores each value at most once and answers "is it in here?" in constant time on average.

```rust
use std::collections::HashSet;

let mut seen = HashSet::new();
seen.insert(3);                  // true: it was new
seen.insert(3);                  // false: already there
seen.contains(&3);               // true
```

`insert` returns a `bool`, which makes "have I seen this before?" a one-liner. Sets support algebra, each yielding an iterator:

```rust
a.intersection(&b)   // in both
a.union(&b)          // in either
a.difference(&b)     // in a but not b
```

A `HashSet` has **no defined order**. If you need a stable result, collect and sort.

## The entry API

The naive way to count words does two lookups: check whether the key exists, then update or insert. `entry` does it once:

```rust
let mut counts: HashMap<String, u32> = HashMap::new();
for w in words {
    *counts.entry(w.to_string()).or_insert(0) += 1;
}
```

`entry(key)` returns a handle to a slot that may be empty; `or_insert(default)` gives a `&mut V` to it, creating the value if needed. Grouping is the same idea:

```rust
groups.entry(w.len()).or_default().push(w);   // or_default uses Default::default()
```

Other useful forms: `.and_modify(|v| *v += 1).or_insert(1)` and `or_insert_with(|| expensive())`.

## This lesson's exercise

Counting, grouping and set algebra with deterministic output (sort before you compare).
