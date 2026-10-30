---
title: Count the Misses
summary: Meet a simulated cache. Reorder a loop so it walks memory in order, and prove the gain by counting misses instead of timing it.
minutes: 25
difficulty: easy
tags: performance, cache, locality, model
id: c-performance/count-the-misses
mode: harness
---
Performance claims need evidence, and a stopwatch is poor evidence: the result changes with the machine, the other programs running, the CPU frequency and whether the run was the first or the tenth. This course measures with **deterministic models** instead.

## A model is not a hardware measurement

Every lesson gives you instruments written in plain C: counters for comparisons and allocations, and a **simulated cache**. They count defined events and give the same answer on every run. That makes them good judges of *why* one design does less work than another.

They are also models. The simulated cache below has one level, 4 KiB, and no prefetcher, no write buffer and no other programs. A real CPU has several levels, hardware prefetching and speculative execution. So read a result like "this loop has 16 times fewer misses" as "in this model, this loop reuses cache lines better", which is a strong hint about real machines and not a promise about them. If a change matters, time it on the real target at the end, and treat that as a sanity check.

## The simulated cache

Memory is fetched in **lines** of 64 bytes. The cache holds 16 *sets* of 4 *ways* each (64 lines, 4 KiB). The line that holds address `a` can only live in set `(a / 64) % 16`. When a set is full, the least recently used line is dropped.

The code in your file wraps each memory access:

```c
int  ld(const int *p);        // read *p and touch its line
void st(int *p, int v);       // write *p and touch its line
```

`cache.accesses` counts every `ld`/`st`, and `cache.misses` counts the ones whose line was not resident. Two ints that share a line cost one miss in total.

## Two loops, same arithmetic

A 64 x 64 grid of `int` is 16 KiB, four times the cache. C stores it **row by row**: `grid[r][0]`, `grid[r][1]`, ... are neighbours, and `grid[r][c]` and `grid[r+1][c]` are 256 bytes apart.

Summing a column at a time visits `grid[0][c]`, `grid[1][c]`, ... Each access lands on a new line, and the whole column does not fit, so by the time you come back for `c + 1` the line you loaded is gone. All 4096 accesses miss. Summing row by row uses all 16 ints of a line before leaving it: 4096 / 16 = 256 misses, which is the least possible (every line must be loaded once).

## Your turn

`col_sums` computes the sum of every column. It is correct and slow, in this model. Change the **loop order** only. (`out[]` holds 64 running totals; we treat them as registers and do not count them as memory.)
