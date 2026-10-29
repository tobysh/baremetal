---
title: SSTables
summary: Write sorted entries to an immutable file with an index at the end, and look keys up with a single read.
minutes: 50
difficulty: hard
tags: sstable, index, io
id: go-database/sstable
mode: harness
---
A **sorted string table** (SSTable) is an immutable file of key-value entries sorted by key. Once written it is never edited. That makes it easy to reason about, cheap to cache, and perfect for sorted output such as a flushed skip list or a compacted log.

Reading a sorted file by scanning from the start is slow. So the table ends with an **index**: for every key, the offset and length of its entry in the file. Open the table, load the index into memory, and finding a key is a binary search in memory followed by **one** read at a known offset.

## Layout

```
[ entry 0 ][ entry 1 ] ... [ entry N-1 ][ index ][ footer ]

entry : flag u8 (1 = tombstone) | keyLen u32 | valLen u32 | key | value
index : for each entry:  keyLen u32 | key | offset u64 | length u32
footer: indexOffset u64 | count u64 | magic u64      (24 bytes)
```

All integers are big-endian. `length` is the whole entry size, header included. The footer has a fixed size and sits at the very end, so a reader can find it knowing only the file size: read the last 24 bytes, check the **magic** number (`0x424D535354303031`, the text "BMSST001"), and the footer says where the index starts.

## Random access: io.ReaderAt

A file you can jump around in is an `io.ReaderAt`:

```go
ReadAt(p []byte, off int64) (n int, err error)
```

It fills `p` from byte `off`. Unlike `Read` it has no hidden position, so many lookups can run at once. One quirk: `ReadAt` may return `n == len(p)` **together with** `io.EOF` when the read ends exactly at the end of the data. Treat that as success:

```go
n, err := r.ReadAt(buf, off)
if n < len(buf) { return err }  // a short read is the real failure
```

`*bytes.Reader` implements `io.ReaderAt`, so tests can use plain memory.

## Validate what you open

A table can be cut short or from another program. `OpenTable` must return an error, not panic, if the file is smaller than a footer, the magic is wrong, the index offset points outside the file, or the index does not parse to exactly `count` entries.

## Sorted input

The writer needs strictly ascending keys: no duplicates (a table has one entry per key), no disorder (binary search would silently break). Return an error rather than writing a broken table.
