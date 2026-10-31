---
title: Pruning Repeated States
summary: Visit each distinct state once with a breadth-first search and a hash table, so retry loops terminate and bigger programs verify.
minutes: 45
difficulty: hard
tags: model-checking, bfs, state-space
id: c-model-checking/prune-states
mode: harness
---
Different schedules often reach the **same state**: two threads that each touch their own variable end in the same place whichever goes first. If you remember the states you have seen, you explore each one once.

## Cycles

A lock-free **retry loop** (`CAS`, and `JZ` back to the start when it failed) can revisit a state it has been in before. Without memory, a depth-first search can run forever on a cycle, or until the depth limit; with a visited table it terminates, because there are finitely many states.

## Breadth-first search

Use a queue of states in order of discovery, and for each new state remember its parent and the thread that led to it. Two properties make this the right tool:

- States leave the queue in order of shortest path.
- If you expand threads in ascending order, equal-length paths are discovered in lexicographic order, so **the first failing state removed from the queue is the minimal counterexample** from lesson 6, found without repeating work. (The first path to a state is its shortest, lexicographically first path, and any minimal failing schedule goes through such a path.)

Rebuild the schedule by following the parent links back from the failing state.

## Counting states

`r->states` is the number of distinct states discovered. For a program with no failure this is the number of reachable states, which you can compare to an independent count.

State equality is `memcmp` of the whole `state`; that is valid because `state` contains only `int` fields with no padding. A hash table gives lookups in constant time. The provided solution caps the table at 40000 states; a bigger model needs a bigger cap, or an honest "unknown" result.
