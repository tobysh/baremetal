---
title: Compaction
summary: An append-only log grows forever. Squash it to just the latest value of each live key.
minutes: 30
difficulty: medium
tags: compaction, log
id: go-database/compaction
mode: harness
---
An append-only log never forgets. Update the same key a million times and the log holds a million records, though only one matters. Delete a key and its tombstone sits beside the old values forever. Replay gets slower and the disk fills up.

**Compaction** rewrites the log keeping only what is still needed:

- For each key, keep only the **newest** record.
- If that newest record is a tombstone, the key is deleted. A tombstone can only be dropped when nothing older than the log you are compacting could still hold the key. Compacting a whole log, you know that. Compacting just part of a larger set of files, you do not: the tombstone must stay to hide the older value elsewhere.

This is why `Compact` takes a `dropTombstones` flag.

## Sorted output

Once there is only one record per key, you are free to choose the order. Writing them sorted by key costs a `sort` and is useful: sorted data can be searched and merged efficiently, which the later lessons will exploit. Real engines do exactly this.

## Entries

From here on the course uses one record type for everything:

```go
type Entry struct {
	Key    string
	Value  []byte
	Delete bool // a tombstone; Value is ignored
}
```

`EncodeEntries` and `DecodeEntries` are given: they turn a slice of entries into bytes and back, rejecting truncated input with an error.

## Don't change what you're given

`Compact` should not modify its input slice or the entries in it. A compactor usually runs in the background while the old data is still being read. Build a fresh result.
