---
title: Term-Based Leader Election
summary: Elect at most one leader per term with votes, step down on seeing a higher term, and refuse candidates whose log is behind.
minutes: 45
difficulty: hard
tags: distributed, consensus, leader-election
id: go-distributed/leader-election
mode: harness
---
Replication is simplest when one node, the **leader**, decides the order of changes. The hard part is choosing a leader when messages are lost and nobody has a trustworthy clock. The Raft-style answer uses **terms**.

## Terms

A **term** is a number that only grows. Every message carries the sender's term, and every node keeps the highest term it has seen.

- A message with a **higher** term than yours means you are out of date: adopt the term, forget any vote you cast in the old term, and become a follower.
- A message with a **lower** term is stale: reject it, and tell the sender your term so that it can step down.

## Elections

A follower whose leader goes quiet starts an election: it adds one to its term, becomes a **candidate**, votes for itself and asks everyone else for a vote. A node grants **at most one vote per term**. A candidate that collects votes from a **majority** (`N/2 + 1`) becomes leader of that term. Two majorities of the same cluster share a node, and that node voted once, so two leaders cannot be elected in one term. That is **election safety**.

## The log restriction

A node refuses to vote for a candidate whose log is *less up to date* than its own: compare the term of the last entry first, then the length. This is what stops a node that missed committed entries from becoming leader and overwriting them. In this lesson the log is only summarised by `LastIndex` and `LastTerm`.

## Things that go wrong

- Counting the same vote twice (a duplicated reply) can crown a leader that has no majority.
- A reply that arrives late, from an older election, must not count.
- Granting a second vote in the same term after hearing from a different candidate breaks election safety.

## Limits of this model

Nothing here decides *when* to start an election. The hidden harness fires timeouts at random, and a real system uses randomised timers so that one node usually times out first. Liveness (a leader eventually exists) is only checked on a reliable network. Lesson 6 covers what must be on disk for these promises to survive a restart.
