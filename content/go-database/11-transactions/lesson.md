---
title: Atomic Transactions with a WAL
summary: Group several writes into one all-or-nothing unit by committing them as a single checksummed log frame.
minutes: 45
difficulty: hard
tags: transactions, wal, atomicity
id: go-database/transactions
mode: harness
---
Moving money means two writes: subtract from one account, add to another. If the machine dies between them, money has vanished. What you want is **atomicity**: after a crash either both writes are there or neither is.

A **write-ahead log** (WAL) gives it to you in this crash model, using the [checksummed frames](/learn/go-database/checksums): a frame is either completely present with a good checksum, or it does not count. So a transaction's whole batch of operations goes into **one frame**, written in one go when you call `Commit`. A physical tail may contain half a frame after a crash; recovery ignores that incomplete frame, so no half-transaction becomes visible. The exercise simulates crashes by truncating an in-memory byte slice. A file-backed engine would also need to handle write failures and synchronize durable bytes before promising durability.

```
Begin  ->  Put, Put, Delete (kept in memory only)  ->  Commit: one frame appended, then applied
                                                  ->  Rollback: forget it, nothing is written
```

## Order of events

`Commit` appends the frame to the log **first**, and only then changes the in-memory data. That is the "write-ahead" rule: the log always knows at least as much as memory does. If the process dies after the append, recovery replays the frame and the transaction survives; if it dies before, the transaction never happened.

## What a transaction sees

A transaction should see its own uncommitted writes (a `Get` after a `Put` in the same transaction returns the new value, and after a `Delete` returns not found), while the database itself and other readers see nothing until commit. Keep the pending operations in an ordered slice and search it **from the end**.

## The payload

The starter gives you `frame` and `unframe` (the CRC frame from earlier). A batch payload is `count u32`, then for each operation `op u8` (1 put, 2 delete), `keyLen u32`, `valLen u32`, key, value. Nothing is stored for an empty transaction: committing it writes no frame.

## Recovery

`Recover` reads frames from the start and applies each batch in order. It stops at the first frame that is short, corrupt or has an undecodable payload. It must also set the new database's log to the **valid prefix** only, so that further commits are not hidden behind garbage.

These transactions implement atomic batches and read-your-writes in a single-threaded engine. Reads that are not covered by pending operations see the current committed database; there is no snapshot isolation, conflict detection or support for concurrent goroutine access. [Snapshots](/learn/go-database/snapshots) explore stable historical reads separately.
