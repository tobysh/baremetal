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
- **Stable**: rows with equal keys keep input order.
- NULL is smaller than any value: first when ascending, last when descending (this is a choice; databases differ, and the track states it).

## Merge join

With duplicates, equal keys on both sides form *groups* and every left row of the group pairs with every right row. Buffer the right group in memory once, replay it for each left row of the same key, then move on. NULL keys never match: skip them on both sides.

The test of an honest merge join is the counter: each input is opened once and never read more than once. A merge join given **unsorted** input silently returns wrong results; the operator trusts its caller, as real engines do.

*Limits:* sort is in memory only; there is no spilling to disk. A column never mixes int64 and string.
