---
title: The LSM Read Path
summary: Combine a memtable with a stack of immutable sorted tables, and read newest to oldest so that updates and deletes win.
minutes: 40
difficulty: hard
tags: lsm, memtable, tombstones
id: go-database/lsm-reads
mode: harness
---
You now have the two halves of a **log-structured merge tree** (LSM tree), the design behind LevelDB, RocksDB and Cassandra:

- A **memtable**: a sorted in-memory structure taking all the writes (a skip list in real engines; a map here is enough to show the idea).
- A stack of **SSTables**: immutable sorted files on disk, each one a frozen memtable.

Writes only ever touch the memtable (and a log for safety). When the memtable gets big, it is **flushed**: its entries are sorted and written out as a new table at the top of the stack, and the memtable starts empty. Nothing is ever updated in place.

## Reads: newest wins

The same key can now be in several places: the memtable has the latest value, and a few older tables hold stale ones. So a lookup searches **from newest to oldest** and takes the first answer:

```
memtable  ->  table 3 (newest)  ->  table 2  ->  table 1 (oldest)
```

A **delete** is a tombstone entry. If the first answer found is a tombstone, the key is gone, and the search must **stop right there**. Carrying on to older tables would bring a deleted value back to life. A tombstone is an answer, just like a value.

## Cost

A key in the memtable costs no table reads. A key in the newest table costs one. A key that does not exist costs one read per table, which is why real engines add Bloom filters (a later lesson) to skip tables cheaply.

The starter gives you a very small in-memory `table` with a `get` method that counts its calls in `tableReads`, so the tests can check that your read path really stops early.
