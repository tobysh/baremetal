---
title: Durable State and Restarts
summary: Decide what a node must write to disk before replying, so that a crash and restart cannot make it break a vote or forget an acknowledged entry.
minutes: 40
difficulty: hard
tags: distributed, durability, recovery, raft
id: go-distributed/durable-restart
mode: harness
---
A reply is a **promise**. "Granted" tells a candidate "I will not vote for anyone else this term". "Success" tells a leader "this entry is stored here". If the node crashes after replying and forgets, it can break the promise:

- It votes for A in term 5, restarts with an empty memory, and votes for B in term 5. Now both can collect a majority: two leaders in one term.
- It acknowledges entry 7, restarts without it, and the leader, which counted that acknowledgement towards a majority, has committed an entry that only exists on fewer nodes than it thinks.

## What must be durable

The Raft-style state that must be on disk **before** the reply leaves the node is:

1. `Term`, the highest term seen;
2. `VotedFor`, whom this node voted for in that term;
3. the `Log`.

State that can be rebuilt is **not** persisted. The commit index is one: after a restart it is 0 and the node learns the real value from the next leader message. Persisting less is fine; persisting more is only slower.

## Ordering and failure

- Change the state, write it, *then* reply. The reply must reflect only what is durable.
- A write can fail. Then you must not reply, and your in-memory state must not run ahead of the disk. The simplest correct response is to reload from disk, as if the node had crashed at that moment.
- Do not write when nothing changed. A heartbeat in the current term leaves term, vote and log as they were.

A subtle one: a **vote request for a higher term** changes the term (and clears the vote) even when the vote is refused. That is a state change too.

## What is supplied

`Node` is the pure in-memory logic of lessons 3 and 4 combined. It knows nothing about disks. `Server` wraps a `Node` and a `Disk`. Node ids are 1, 2, 3, ... and `VotedFor == 0` means "nobody", so a blank disk reads back as a fresh node.

## Limits of this model

The disk is atomic: a `Save` either completes or leaves the old state. Real storage can tear a write or lie about `fsync`; lesson 2 of the database course (`go-database`) is about surviving torn writes. Here one `Save` writes the whole state, which is simple but would be slow for a big log.
