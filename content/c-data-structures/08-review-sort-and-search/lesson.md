---
title: "Review: Sort and Search Toolkit"
summary: A consolidation exercise. Combine a copy-and-sort, insertion sort and binary bounds into three small statistics tools. No new ideas.
minutes: 25
difficulty: medium
kind: review
tags: sorting, searching, arrays
id: c-data-structures/review-sort-and-search
mode: harness
---
You have written [binary search](/learn/c-data-structures/binary-search), [insertion sort](/learn/c-data-structures/insertion-sort), [merge sort](/learn/c-data-structures/merge-sort) and [quicksort](/learn/c-data-structures/quicksort). This review puts the ideas together without adding anything new.

Many questions about a collection are easy once it is sorted:

- Is it already sorted? One pass comparing neighbours.
- What is the middle value (the **median**)? Sort, then read the middle.
- How many values lie between two limits? Sort, then use two binary searches: the first index with `>= lo` and the first index with `> hi`. The difference is the count.

The twist is that the caller's array is `const`: you must not reorder it. Work on a **copy**.

```c
int *copy = malloc((size_t)n * sizeof *copy);
if (copy == NULL) return -1;
memcpy(copy, a, (size_t)n * sizeof *copy);
// ... sort copy, search copy ...
free(copy);
```

Pick whichever sort you like (insertion sort is the shortest to write; `qsort` from `<stdlib.h>` is allowed too). Remember to free the copy on every path that allocated it.

Write the three helpers.
