---
title: Range Scans with Merge Iterators
summary: Read a key range across several sorted sources at once, lazily, with the newest version of each key winning.
minutes: 50
difficulty: hard
tags: iterators, merge, range-scan
id: go-database/merge-iterators
mode: harness
---
Point lookups ask one question. A **range scan** asks "give me everything from `user:100` up to, but not including, `user:200`, in order". In an LSM tree the answer is spread over the memtable and many tables, each sorted, each possibly holding a different version of the same key. A scan has to **merge** them.

## A small iterator interface

```go
type Iterator interface {
	Next() (Entry, bool) // the next entry, or false when finished
}
```

An iterator hands out entries one at a time and does only as much work as you ask for. That laziness matters: a scan of the first ten keys must not read a billion entries first.

## Merging sorted sources

Take several sorted iterators, **ordered from newest to oldest**. To produce the merged order, remember the next unread entry of each source (its *head*). Each step:

1. Find the smallest key among the heads.
2. The winner is the head with that key from the **lowest-numbered (newest) source**.
3. Advance **every** source whose head has that same key: older versions of the key are shadowed and must be dropped.
4. Return the winner.

Tombstones pass through the merge untouched, because a tombstone in a newer source has to hide a value in an older one.

## Filtering as separate layers

Two more small iterators wrap any other:

- `Live` drops tombstones, so callers only see real data. It must come **after** the merge. If you drop tombstones first, the old value they were hiding reappears.
- `Range(it, start, end)` skips keys below `start` and stops at the first key `>= end`. An empty `end` means no upper bound. It must stop **at once** instead of draining the source, which is what makes a short range over a huge table cheap.

Because each layer only wraps `Next()`, you can stack them: `Range(Live(Merge(mem, t3, t2, t1)), a, b)`.
