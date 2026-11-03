---
title: Capstone: Replay a Metadata Journal
summary: Recover committed metadata transactions idempotently after every modeled crash point.
minutes: 40
difficulty: hard
id: go-filesystems/journal-recovery
---
A redo journal records intended metadata values before installing them at their home locations. In this model a transaction contains a complete record list followed by one atomic commit marker. A crash before the marker leaves no committed transaction; a crash after it requires replay. Home writes can fail partway through replay. Keep the committed journal until every home write succeeds, so the next restart can safely retry.

This simplified journal is a supplied durable object, not a byte-level log format. Records and the commit marker are assumed atomic and ordered. Checksums, journal reuse, concurrent transactions and torn writes require additional machinery outside this model.

Run the starter to inspect the public example, then implement the contract in the task. The reference implementation produces the specified results for these same inputs.
