---
title: "Review: Array Toolkit"
summary: A consolidation exercise. Combine looping, searching and in-place writing in four small array tools. No new ideas.
minutes: 20
difficulty: medium
kind: review
tags: arrays, algorithms
id: c-data/review-arrays
mode: harness
---
This is a **review lesson**. It introduces nothing new. Each function combines ideas you already used, with a small twist.

## What you already know

- **[Arrays and Indexing](/learn/c-data/arrays):** loop over `a[0]` to `a[n - 1]`, accumulate a total, track a maximum.
- **[Searching and Checking Arrays](/learn/c-data/searching):** compare neighbours (`a[i]` with `a[i + 1]`), return early, and count matches.
- **[Changing Arrays in Place](/learn/c-data/changing-arrays):** write into `a[i]`, keep a second index for where the next value goes.

## The twists

1. **Running totals.** `prefix_sums` writes into a *second* array: `out[i]` is the sum of `a[0]` through `a[i]`. You can keep a running total while you loop.
2. **A maximum, but not the biggest.** The second largest *distinct* value needs two variables: the best so far and the runner-up that is strictly smaller.
3. **Two arrays at once.** `arrays_equal` walks both arrays with the same index.
4. **Count the neighbours.** `count_peaks` counts elements that are bigger than both neighbours. The first and last element have only one neighbour, so they never count; start and stop the loop accordingly.

If one feels shaky, reread the lesson it came from.
