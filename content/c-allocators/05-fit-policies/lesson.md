---
title: Fit Policies
summary: Add best fit next to first fit, count the blocks each search examines, and compare the policies on the same workloads with deterministic counters.
minutes: 40
difficulty: hard
tags: heap, first-fit, best-fit, benchmarking
id: c-allocators/fit-policies
mode: harness
---
The free-list heap of the last lesson always took the **first** block that fit. That is one *policy*. When several free blocks could serve a request, which one should you pick?

- **First fit**: the lowest-address block that is big enough. The search stops early, so it is usually fast. Small leftovers tend to pile up near the start of the heap.
- **Best fit**: scan *all* free blocks and take the smallest one that fits (lowest address on a tie). It leaves the smallest leftover, so it keeps big holes intact. It always scans everything (this heap has no size-ordered index), and it tends to create many tiny, useless remainders.

Neither wins everywhere. You can only compare them by running the **same operation sequence** through each and looking at counters.

## Counters, not clocks

Wall-clock time depends on the machine and noise, so it is a poor oracle in tests and in lessons. Instead count events that the algorithm itself defines:

- `search_steps`: how many blocks a search examined (used or free, every block visited counts once, including the one chosen).
- `failed_allocs`: requests (with `n > 0`) that returned `NULL`.
- Afterwards: `free_blocks` and `largest_free` from the stats.

The same input always gives the same counters, so a test can demand exact numbers, and you can reason about them by hand.

## A trace where the policies diverge

Two holes of 512 and 256 bytes (in that address order), nothing else free. Request 200 bytes, then 300 bytes.

- First fit puts the 200 in the 512 hole (first one that fits), leaving 296 plus the 256 hole. 300 fits neither: **failure**.
- Best fit puts the 200 in the 256 hole, keeping the 512 hole for the 300: **both succeed**.

Reverse the hole order or the request sizes and first fit can win instead. A fair statement is "on this workload ...", never "best fit is better".

## What this lesson cannot tell you

These counters model a linked-list search on a 2 KiB heap with a handful of synthetic workloads. They do not measure cache effects or real programs, and a real allocator's choice also depends on its data structures (size-segregated lists make "best fit" almost free). Whether you can *explain* a difference in the numbers is not machine-graded; the exercise at the end of the task asks you to do that in a comment.

## Your turn

The starter has a working first-fit heap. Add the best-fit search and the step counter in `find_block`, then write `run_workload`, which replays an operation list under one policy and reports the counters.
