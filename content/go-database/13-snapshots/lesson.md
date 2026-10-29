---
title: Snapshots and MVCC-lite
summary: Keep several versions of each key so a reader sees a frozen, consistent view while writers carry on.
minutes: 50
difficulty: hard
tags: mvcc, snapshots, versions
id: go-database/snapshots
mode: harness
---
Imagine a long report that reads a thousand keys while other code keeps writing. Halfway through, the data changes under it and the report adds up numbers that never existed at the same moment. A **snapshot** is a frozen view of the database: reads through it keep returning the data as it was when the snapshot was taken, no matter what is written afterwards. This exercise interleaves reads and writes sequentially: the store is not safe for concurrent goroutine access without additional synchronization.

The classic way to do this is **multi-version concurrency control** (MVCC), and a small version of it is not hard.

## Sequence numbers

Give every write a number from a counter that only goes up. Instead of overwriting a key, **append a version** to that key's list:

```go
type version struct {
	seq     uint64
	value   string
	deleted bool
}
// "k": [ {seq 3, "a"}, {seq 7, "b"}, {seq 9, deleted} ]
```

A normal `Get` reads the newest version. A snapshot remembers the counter's value at the moment it was taken, and its `Get` reads the newest version **with `seq <=` that number**: later writes are invisible.

```
store.Put("k","a")   // seq 1
snap := store.Snapshot()   // sees up to seq 1
store.Put("k","b")   // seq 2
snap.Get("k")  -> "a"      store.Get("k") -> "b"
```

A deleted version is a tombstone version. Nothing is destroyed, so older snapshots still see the value.

## Garbage

Versions pile up. An old version can be thrown away only when **no snapshot can see it any more**. Find the *oldest* live snapshot's sequence number `s` (or the current counter when there are no snapshots). For each key:

- Keep the newest version with `seq <= s`. That is what the oldest snapshot reads.
- Keep every version newer than that, since newer snapshots, or the present, may read them.
- Drop everything older.
- If the version kept as the oldest is a tombstone, drop it too, since a key that is deleted from everyone's point of view needs no record. If no versions remain, remove the key entirely.

Snapshots therefore must be **closed**; an open snapshot pins old versions in memory, which is the price of consistency.

This completes the storage-engine building blocks. The [durable-store review](/learn/go-database/review-durable-store) combined the map, recovery, CRC and compaction. [SSTables](/learn/go-database/sstable), [LSM lookup](/learn/go-database/lsm-reads), [Bloom filters](/learn/go-database/bloom-filter), [atomic batches](/learn/go-database/transactions), [merge iterators](/learn/go-database/merge-iterators) and version visibility solve different parts of a larger engine; these standalone exercises do not yet integrate them into one file-backed, concurrently accessible database.
