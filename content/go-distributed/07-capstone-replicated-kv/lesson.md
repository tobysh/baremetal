---
title: Capstone: A Replicated Key-Value Store
summary: Apply only committed log entries, preserve retry identities, and reconstruct a store after a simulated restart.
minutes: 50
difficulty: hard
tags: distributed-systems, replication, capstone
id: go-distributed/capstone-replicated-kv
---
Replication and application are separate steps. A leader can hold an entry that no majority has accepted. Making that entry visible would let a partition expose data that is later discarded.

The supplied leader and follower models implement the replication checkpoint. Your state machine consumes only the committed prefix. Commands carry stable request IDs; retries with the same ID must not overwrite the first committed result, even if their payload differs. This makes deduplication part of the replicated state rather than a temporary cache.

On restart, create an empty store and replay the committed prefix to rebuild both values and retry identities. The caller supplies the known commit boundary; this exercise does not implement election recovery, persistent commit discovery, client transport or linearizable reads. It demonstrates deterministic application after replication, within the earlier simulated leader model.

Run the starter to inspect the public example, then implement the contract in the task. The reference implementation produces the specified results for these same inputs.
