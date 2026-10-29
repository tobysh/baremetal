---
title: "Review: A Durable Store"
summary: A consolidation exercise. Combine the log, checksummed frames, replay with a torn tail, and compaction into one small database. No new ideas, just practice.
minutes: 30
difficulty: medium
kind: review
tags: log, crc, recovery, compaction
id: go-database/review-durable-store
mode: harness
---
This review combines the [key-value map](/learn/go-database/kv-store), [log recovery](/learn/go-database/recovery), [CRC frames](/learn/go-database/checksums) and [compaction](/learn/go-database/compaction).

This is a **review lesson**. It introduces nothing new. In the last four lessons you wrote a length-prefixed record, replayed a log to rebuild a map, protected records with a CRC, and compacted a log down to the live keys. Here you join them into one tiny database, `DB`, that lives entirely in a byte slice.

## What to combine

- Every operation is appended to the log as one **frame**: `crc u32`, `len u32`, `payload`, big-endian, the CRC (IEEE) covering the length bytes and the payload.
- The payload is `op u8` (1 = put, 2 = delete), `keyLen u32`, then the key bytes, then the value (the rest of the payload).
- `Open` replays the log. It stops at a frame that is short **or** has a bad checksum, keeps everything before it, and **discards** the damaged tail from its own copy of the log so later appends follow good data.
- `Compact` rewrites the log with one put frame for each live key, sorted by key.

The twist compared with earlier lessons is that the checks live in one place: a single loop reads frames, and a single path writes them. Keep the encoding in two small helpers and the rest follows.
