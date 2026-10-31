---
title: Log Replication: AppendEntries
summary: Build the follower side of a replicated log. Check the previous entry, truncate only real conflicts, and clamp the commit index.
minutes: 45
difficulty: hard
tags: distributed, replication, raft, log
id: go-distributed/append-entries
mode: harness
---
A leader orders client commands in a **log** and sends them to followers. Followers must end up with the same log, even though requests can be lost, duplicated, delayed and reordered, and an old leader may still be sending.

## The consistency check

Log indexes start at 1. Each entry has the **term** in which a leader created it. Every `AppendEntries` request says: "these entries go after index `PrevIndex`, and the entry there has term `PrevTerm`".

- If the follower has no entry at `PrevIndex`, or it has a different term, it **rejects**: it cannot be sure its history matches the leader's. The leader then tries an earlier `PrevIndex`.
- If it matches, then (by induction) everything before `PrevIndex` matches too, and the follower may take the new entries. This is the **log matching property**: two logs with the same term at the same index are identical up to there.

`PrevIndex` 0 always matches (the empty prefix).

## Merging entries

Do not simply cut the log at `PrevIndex` and append. A **duplicated or delayed** request can carry fewer entries than the follower already holds, and truncating would throw away entries that were acknowledged. Instead go through the request's entries one by one:

- the follower has an entry at that index with the **same term**: it is the same entry, keep it;
- it has one with a **different term**: a genuine conflict, so delete it and everything after it, then append the rest;
- it has none: append.

## Terms and commit

A request with a term lower than the follower's is from a deposed leader: reject it and report the current term. A higher term is adopted.

`LeaderCommit` is the highest index the leader knows to be safely replicated. The follower may mark entries committed up to `min(LeaderCommit, PrevIndex + len(Entries))`: only entries *this request verified* are known to match the leader, and the follower's log may hold extra, unverified entries after them. The commit index never decreases.

## The reply

`Success` says whether the request was accepted. `Match` is, on success, the last index the request verified (`PrevIndex + len(Entries)`). On failure it is a **hint** for the leader's next try: `min(len(log), PrevIndex-1)`, the last index at which the follower could possibly agree.

## Limits of this model

This is the follower only; the leader that generates requests is lesson 5, and persistence is lesson 6. The model has no snapshots, so logs only grow.
