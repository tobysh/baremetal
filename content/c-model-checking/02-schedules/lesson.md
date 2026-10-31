---
title: Schedules and Interleavings
summary: Replay a schedule exactly, reject illegal ones, and count every way the threads can interleave with a depth-first search.
minutes: 30
difficulty: easy
tags: model-checking, interleaving, dfs
id: c-model-checking/schedules
mode: harness
---
With `thread_enabled` and `step` working (they are provided now), you can run any schedule and, more usefully, **all** of them.

## Replaying a schedule

`run_schedule` starts from the initial state and takes the steps in order. A schedule is only legal if each named thread is enabled when its turn comes: you cannot step a finished thread or a thread blocked on a lock. When a step is illegal the function returns `-(i+1)`, where `i` is the index of that step, so that `0` is never confused with a failure.

## Counting interleavings

Two threads with 3 steps each can interleave in `C(6,3) = 20` ways. Three threads of 3 steps give `9! / (3!)^3 = 1680`. The count grows explosively, which is why models stay small. Locks *remove* schedules because a blocked thread cannot move.

The search is a **depth-first search**: from a state, try each enabled thread in turn, copy the state (it is a plain struct, so `state next = *s;` is a copy), take the step and recurse. A state where every thread is at `OP_END` is one complete schedule. A state where some thread is unfinished but none can move is a *dead end*; it is not complete.

This counting works only for programs without loops, since a retry loop has infinitely many schedules (lesson 7 handles that).
