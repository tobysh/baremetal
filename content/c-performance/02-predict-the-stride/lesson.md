---
title: Predict the Stride
summary: Work out on paper how many cache misses a strided scan causes, then check the prediction against the simulated cache, including set conflicts.
minutes: 25
difficulty: medium
tags: performance, cache, stride, prediction
id: c-performance/predict-the-stride
mode: harness
---
Lesson 1 measured a change. Here you **predict** before measuring, which is the habit that stops you believing a number just because it is printed.

## The cache from lesson 1

Lines of 64 bytes, 16 sets, 4 ways, LRU, 4 KiB in total. Line number `L = address / 64` lives in set `L % 16`. As before, this is a model: no prefetcher, one level, no other traffic.

## Scanning with a stride

`scan(n, stride)` reads `data[0]`, `data[stride]`, `data[2*stride]`, ... while the index is below `n`, on a cold cache. `int` is 4 bytes, so a line holds 16 consecutive ints.

- With `stride = 1` each line serves 16 reads: about `n / 16` misses.
- With `stride = 4` a line serves 4 reads: about `n / 16` lines are still touched, because the scan still walks through every line.
- With `stride >= 16` every read is on a different line: one miss per read, `ceil(n / stride)` misses.

Fewer reads do not mean fewer misses until the stride passes a line. The exact count is the number of **distinct lines** that the visited indices fall on. With a stride under one line no line is skipped, so that count is the line of the last visited index plus one.

## Sets and conflicts

Only 4 lines fit in each set. If the stride in lines shares a factor with 16, the scan keeps landing in a few sets. Stride 16 ints is 1 line: all 16 sets. Stride 2 lines visits only the even sets (8 sets), stride 16 lines is **always the same set**, so only 4 lines are resident however long the scan is. In general a scan with stride `s` lines touches `16 / gcd(16, s)` sets. Real caches are indexed the same way, which is why array sizes that are powers of two can be unexpectedly slow, and why that is also a modelling choice: some real caches hash the index.

## Your turn

Write two small functions. They only use arithmetic; you do not touch the cache in them.

- `predict_misses(n, stride)`: the misses of `scan(n, stride)` on a cold cache, for `n <= 1024` (the whole array fits, so nothing is evicted).
- `sets_touched(stride_lines)`: how many different sets a long scan touches when each step moves `stride_lines` whole lines.

The tests run the real simulated cache and compare. A formula that is only right for strides you tried by hand will fail on the others.
