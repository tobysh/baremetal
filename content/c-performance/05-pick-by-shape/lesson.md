---
title: Pick by Shape
summary: Count the sorted values two arrays share. Choose a merge or repeated binary search by input shape, and prove the comparison count stays under the better bound.
minutes: 30
difficulty: medium
tags: performance, algorithms, counters, complexity
id: c-performance/pick-by-shape
mode: harness
---
Layout and allocation change *constants*. An algorithm changes how work **grows** with the input. Counters show both.

## The problem

Given two sorted arrays of distinct ints, `a` (length `na`) and `b` (length `nb`), count the values that appear in both.

- **Nested loops**: compare every pair: `na * nb` comparisons. For two arrays of 2000 values that is 4 million.
- **Merge**: walk both arrays with two indices. Each comparison advances at least one index, so at most `na + nb` comparisons: 4000 for the same arrays.
- **Binary search**: for each element of the *shorter* array, binary-search the longer one: at most `ceil(log2(longer + 1))` comparisons each.

## The shape decides

With `na = 1` and `nb = 4000`, merge costs up to 4001 comparisons while one binary search costs 12. With two arrays of 2000, binary search costs about `2000 * 11 = 22000`, far more than merge's 4000. Neither wins everywhere; the winner depends on the **ratio** of the lengths. You can pick before running anything, from the bounds:

```text
merge:   na + nb
search:  small * ceil(log2(big + 1))
```

## The counter

Every comparison goes through `cmp3(x, y)`, which adds one to `ncmp` and returns -1, 0 or 1. The budget the tests apply for any input is

```text
min(na + nb,  small * (ceil(log2(big + 1)) + 1))
```

(the `+ 1` is slack for a binary-search implementation that tests equality separately). A function that simply sorts or hashes would need extra memory and different counters; here we keep to comparisons, and say so.

## What this does not tell you

Comparison counts are a model of work. Merge reads memory sequentially, which real caches and prefetchers like; binary search jumps around, which they dislike. On real hardware the crossover can sit at a different ratio than the comparison counts suggest. For small arrays a plain loop can be the fastest of all, because constants dominate. The counters tell you what the algorithm must do, not how a machine will schedule it.

## Your turn

`count_common` is correct but slow. Keep it correct and make it respect the budget for every shape.
