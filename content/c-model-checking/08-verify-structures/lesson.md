---
title: Capstone: Verify Two Structures
summary: Write a lock-based, a flawed lock-free and a corrected lock-free push for a ring buffer, then model-check all three and read the counterexample.
minutes: 60
difficulty: hard
tags: model-checking, lock-free, capstone
id: c-model-checking/verify-structures
mode: harness
---
You now have a model checker. Use it on the structure from `c-concurrency`'s last lesson: a ring buffer, reduced to its core operation, **push**.

## The model

`m[0]` is the **tail** index and `m[1..3]` are the slots. Each thread pushes the value `1`. If every push lands in its own slot, the slots sum to the number of threads `n`; a lost push leaves a smaller sum. This is already the program's `fin_*` spec (set by `push_base`, which you must not change).

## Three versions

1. **Locked push.** Take lock 0, read the tail, write the slot at the tail, store tail + 1, unlock.
2. **Flawed lock-free push.** No lock, no CAS: read the tail, write the slot, then store tail + 1 with a plain store. Two threads can read the same tail, write the same slot and lose a push.
3. **Corrected lock-free push.** Claim a slot index with `CAS` on the tail (retry if it fails), and only then write the slot. The index you won is yours alone.

## The task as an investigation

You write the instruction arrays by hand, then run the checker from lesson 7 on each, for 1, 2 and 3 pushing threads. For the flawed push read the counterexample it prints and say, in your own words, which two steps broke the structure. A short written justification cannot be graded by tests, so the tests check the observable parts only: the verdicts, the minimal counterexample against a separate reference, that each push works when threads run one at a time, and that the programs have the required shape.

## What this does and does not prove

"No failing schedule" means every interleaving of these instructions, with these three slots, up to 3 threads, one push per thread, and atomic steps, is correct. It does **not** cover memory-ordering effects on real hardware (see the memory-ordering lesson of `c-concurrency`), more pushes than slots, a consumer, or ABA with reused nodes. The ring has no pop here; adding one is a good extension.

A UI for stepping through counterexamples is future work.
