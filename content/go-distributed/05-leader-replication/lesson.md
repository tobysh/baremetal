---
title: Leader Replication and Commit
summary: Track follower progress, handle stale replies, and commit current-term entries only after majority replication.
minutes: 50
difficulty: hard
tags: distributed, replication, raft, commit
id: go-distributed/leader-replication
mode: harness
---
Lesson 4 built the follower. Now the leader: it must bring every follower's log in line with its own, and decide when an entry is **committed**, meaning it will never be lost whatever fails next.

## What the leader tracks

For each follower:

- `match`: the highest index known to be identical on that follower. Starts at 0.
- `next`: the index of the next entry to send. Starts just after the leader's last entry (optimistic).

A request for follower `p` carries the entries from `next[p]` onward, with `PrevIndex = next[p]-1` and the term of that entry.

## Replies are late, doubled and out of order

- On **success** the reply proves the follower matches up to `req.PrevIndex + len(req.Entries)`. Raise `match` to that if it is higher, never lower it: an older success may arrive after a newer one. `next` must be at least `match+1`.
- On **failure** the follower's `Match` field is a hint: try again from `Match+1`. A failure reply can also be stale, from a request sent when `next` was larger than it is now. So only ever move `next` *down*, and never below `match+1`, which is already proven.
- A reply carrying a **higher term** means a newer leader exists. Mark the leader `Deposed` and stop changing anything.

## When is an entry committed?

An entry is committed once it is stored on a **majority** of nodes (counting the leader). But there is a trap, the one that bites real implementations: a new leader may hold entries from *earlier terms* that are on a majority and still not safe. A later leader whose log lacks them could still be elected on a different majority and overwrite them. So a leader only advances the commit index to an entry **from its own term** that is on a majority. That implicitly commits everything before it.

```
leader (term 3) log: [1:t1] [2:t2]         <- both on a majority, from earlier terms
commit stays 0 ...
after it appends [3:t3] and a majority holds index 3: commit = 3, entries 1 and 2 included
```

## Limits of this model

A deposed leader here just stops. Real systems also stop accepting client commands and may redirect clients. The batch size is a parameter so tests can force many round trips; real systems also bound bytes. Elections (lesson 3) are not connected to replication until the capstone.
