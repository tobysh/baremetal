---
title: Find the Lost Update
summary: Search all interleavings for a final state that violates the expected result, and return one failing schedule as the counterexample.
minutes: 35
difficulty: medium
tags: model-checking, race-condition, counterexample
id: c-model-checking/lost-update
mode: harness
---
A **data race** in real code means two threads touch the same memory without ordering and at least one writes. In our model you cannot observe hardware effects, but the usual symptom is visible: a **lost update**, where the final result is not what any sequential order of the operations would give.

## The oracle

A model checker needs a statement of what is correct. A `program` carries one: when every thread has finished, the shared cells `m[fin_lo..fin_hi]` must sum to `fin_sum` (`fin_lo = -1` means no expectation). For two increments of `m[0]` that is `fin_sum = 2`.

## Counterexamples

If *any* schedule ends in a state that breaks the oracle, the program is wrong, and that schedule is the proof. It is more useful than a test failure: it is a recipe you can replay with `run_schedule`. If no schedule fails, the program is correct **in this model**: every interleaving of the instructions as written has been tried.

`model_check(p, r)` fills a `result`: `verdict` (`V_OK` or `V_LOST_UPDATE` here), `len`, and `sched[0..len)`. This lesson accepts any failing schedule; lesson 6 asks for the minimal one.

Programs here have no loops and no lock cycles, so the search always ends. A state with unfinished threads that cannot move (a deadlock) is not reported yet; that is the next lesson.
