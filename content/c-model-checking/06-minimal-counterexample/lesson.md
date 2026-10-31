---
title: Minimal Counterexamples
summary: Return the shortest failing schedule, and among equal lengths the lexicographically first, so the answer is deterministic and easy to read.
minutes: 45
difficulty: hard
tags: model-checking, counterexample, iterative-deepening
id: c-model-checking/minimal-counterexample
mode: harness
---
Any failing schedule proves a bug, but a 40-step schedule is hard to understand and two runs of a smarter search may return different ones. We fix the answer.

> The **minimal counterexample** is the failing schedule with the fewest steps; among those, the one that is smallest when compared thread id by thread id from the start (`0 0 1 ...` before `0 1 0 ...`).

## Failing states

A schedule fails if the state it reaches is bad, in this order of precedence:

1. the invariant is violated (`V_ASSERT`),
2. all threads are done and the final sum is wrong (`V_LOST_UPDATE`),
3. unfinished threads and none enabled (`V_DEADLOCK`).

## Shortest first

The first DFS hit is not necessarily the shortest. A program can fail by deadlock after 8 steps or by a lost update after 14; plain DFS may find the 14-step one first. **Iterative deepening** fixes this: run a depth-limited DFS with limit 0, then 1, then 2, and so on, and look for a failing state at *exactly* depth `limit`. Because shallower limits found nothing, the first hit is the shortest. Trying threads in ascending order makes the first hit within that depth the lexicographically first.

The alternative is breadth-first search, which lesson 7 uses.

## A safety valve

The provided solution has a node budget so a runaway search stops. If it gives up it reports `V_OK`, which would be a **false proof**. Treat a budget stop as "unknown" in real tools.

## Why it helps

For the racy counter the minimal counterexample is `0 0 1 0 1 1`: thread 0 loads and adds, thread 1 loads the old value, then both finish. Reading it tells you the bug almost by itself.
