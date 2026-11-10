---
title: Capstone: Replay a Metadata Journal
summary: Recover committed metadata transactions idempotently after every modeled crash point.
minutes: 40
difficulty: hard
tags: filesystem, journaling, recovery
id: go-filesystems/journal-recovery
---
Ordering writes carefully (lesson 7) works when one pointer is the single commit point. An operation that must change several metadata blocks together, say a bitmap bit, an inode and a directory, has no single switch. A **journal** (write-ahead log) manufactures one. Before touching the real ("home") locations, the filesystem writes a record of what it intends to do to a separate log area, then writes one **commit marker**. Only after the marker is durable does it install the changes at home.

## The redo journal

```text
 1. write records:   (block 0 := 7)  (block 2 := 9)  (block 3 := -1)
 2. write commit marker                      <- the atomic switch
 3. install at home: Home[0]=7  Home[2]=9  Home[3]=-1
 4. clear the journal
```

A crash decides everything by where it lands:

- **Before step 2:** no commit marker, so the transaction never happened. The home blocks are untouched. Recovery discards the log.
- **After step 2, before step 4:** the transaction is committed but may be half installed. Recovery **replays** every record from the journal. Replay is safe to repeat because each record is an *assignment* ("block 0 becomes 7"), not an increment, so applying a prefix twice gives the same result as applying it once. This is **idempotence**.
- **During recovery itself:** the machine can crash again. That is why the journal must be cleared only after the *last* home write succeeds. If recovery gave up halfway and cleared the log, the half-installed transaction would be lost for good.

## The model

The `Disk` is given:

```go
type Record struct{ Block, Value int }
type Journal struct {
	Committed bool
	Records   []Record
}
type Disk struct {
	Home             []int
	Journal          Journal
	FailAt, Attempts int
}
```

`d.Write(b, v)` is the only way to change `Home`. It counts each call in `Attempts` and returns `ErrCrash` instead of writing when the call number equals `FailAt` (use `FailAt = -1` for a disk that never fails). The tests use this to cut recovery at every home write.

## Pitfalls to design for

- **Validate first.** A committed record with an address outside `Home` is a corrupt journal. If you discover it after writing the first records, home is half changed by a transaction you are about to reject. Check every record before the first write and return `ErrJournal`, leaving the disk and journal untouched.
- **Order matters for duplicates.** Records `{0,7}` then `{0,11}` must leave `Home[0] = 11`: replay in order and last value wins.
- **Stop at the first error** and return it without clearing the journal, so the next attempt can retry the whole list.
- **An uncommitted journal is not yours to apply.** Do nothing, not even clean it up here.
- **A second recovery must do nothing**: no writes, no attempts counted.

Do not reset `FailAt` or `Attempts`; they belong to the disk's fault injection, not to your code.

## Limits of the model

Records and the commit marker are assumed atomic and ordered, which real journals get with checksums and barriers. There is no journal reuse or wrap-around, no concurrent transactions, and no torn writes. Real filesystems (ext4, XFS) also choose whether to journal data or only metadata; this model journals whatever values the records carry.
