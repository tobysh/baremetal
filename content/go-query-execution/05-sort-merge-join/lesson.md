---
title: Sort and Merge Join
summary: Sort rows with stable multi-key ordering and NULLs first, then join two sorted inputs in a single forward pass, buffering only a group of equal keys.
minutes: 55
difficulty: hard
tags: join, sort, merge, duplicates, null
id: go-query-execution/sort-merge-join
mode: harness
---
If both inputs are sorted on the join key, you do not need a hash table or a rescan. Walk two cursors forward: advance whichever side has the smaller key; when the keys are equal, emit the matches.

## Sort

`Sort` is a **blocking** operator. It cannot return its first row before it has read its last input row, so the counters show all rows read after one `Next`. Ordering rules:

- Several keys, each ascending or descending.
- **Stable**: rows with equal keys keep input order, so use `sort.SliceStable`.
- NULL is smaller than any value: first when ascending, last when descending (this is a choice; databases differ, and the track states it).

## Merge join

```text
left (sorted)    right (sorted)
NULL, v          NULL, m
1,    a          1,    y
3,    c          3,    x
3,    d          3,    z
```

```text
left NULL: skip.     right NULL: skip.
1 = 1  -> emit (1,a,1,y)
3 = 3  -> group on the right is [x, z]; emit (c,x) (c,z); next left is also 3: replay the group, emit (d,x) (d,z)
```

With duplicates, equal keys on both sides form *groups* and every left row of the group pairs with every right row. Buffer the right group in memory once, replay it for each left row of the same key, then move on. NULL keys never match: skip them on both sides.

## Advancing the cursors

1. Left key NULL: advance left.
2. Right key NULL or smaller than the left key: advance right.
3. Right key larger: advance left.
4. Equal: read the whole run of equal right rows into a slice, emit left x group, and while the next left row has the same key replay the group.

The test of an honest merge join is the counter: each input is opened once and never read more than once, and once one side is exhausted you stop reading the other. A merge join given **unsorted** input silently returns wrong results; the operator trusts its caller, as real engines do.

*Limits:* sort is in memory only; there is no spilling to disk. A column never mixes int64 and string.
