---
title: Crash Recovery by Replay
summary: Rebuild the in-memory state from the log after a restart, and cope with a half-written record at the end.
minutes: 35
difficulty: medium
tags: log, recovery, durability
id: go-database/recovery
mode: harness
---
The log is the truth. The map is just a cache of "what the log says right now". After a crash or restart you rebuild the map by **replaying** the log from the start: read every record in order and apply it. Later records overwrite earlier ones, so the last write to a key wins.

## Deletes are records too

You cannot erase bytes from the middle of an append-only file, so a delete is another record: a **tombstone** that says "this key is gone". Replaying a tombstone removes the key from the map.

Here each record starts with an operation byte:

```
+------+-----------+-----------+-----+-------+
| op u8| keyLen u32| valLen u32| key | value |
+------+-----------+-----------+-----+-------+
op 1 = put,  op 2 = delete (valLen is 0)
```

`EncodePut` and `EncodeDelete` are given. Your job is the other half.

## The torn tail

A crash can happen in the middle of a write. The file then ends with a **torn record**: a header with no body, half a key, anything. This is expected and must not be an error. The right behaviour is:

1. Apply every complete record before the damage.
2. Stop at the first record that is incomplete (or has an unknown op byte).
3. Report how many bytes were good, so the caller can **truncate** the garbage away before appending again. If new records were written after a torn one they would be unreachable forever, because replay would stop at the garbage every time.

When you check whether a record fits, beware of arithmetic on lengths read from the file. A damaged header can claim a 4 GB key, and `9 + keyLen + valLen` can overflow a 32-bit integer. Do the check in `uint64` or `int64`, and compare against the number of bytes that remain.
