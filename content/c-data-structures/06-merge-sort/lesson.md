---
title: Merge Sort
summary: Divide and conquer. Write the merge step for two sorted arrays, build merge sort on top of it with recursion and a scratch buffer, and keep it stable.
minutes: 30
difficulty: medium
tags: sorting, recursion, algorithms
id: c-data-structures/merge-sort
mode: harness
---
Insertion sort is O(n²). **Merge sort** is O(n log n) and always runs in that time, however the input is arranged. It uses a strategy called **divide and conquer**:

1. Split the array in half.
2. Sort each half (recursively).
3. **Merge** the two sorted halves into one sorted array.

A half of size 1 (or 0) is already sorted, so the recursion stops there.

## The merge step

Merging two sorted arrays is simple: keep an index into each, repeatedly copy the smaller front element to the output, and when one input runs out copy the rest of the other.

```c
size_t i = 0, j = 0, k = 0;
while (i < na && j < nb) {
    if (b[j] < a[i]) out[k++] = b[j++];
    else             out[k++] = a[i++];
}
while (i < na) out[k++] = a[i++];
while (j < nb) out[k++] = b[j++];
```

Taking from `a` on ties (we only take from `b` when it is *strictly* smaller) makes the sort **stable**: equal elements keep their original relative order. Stability matters when you sort records by one field and want a previous ordering by another field preserved.

## Sorting in place needs a scratch buffer

The merge writes into a separate array, so allocate one temporary buffer the size of the data, sort recursively into and out of it, and free it at the end:

```c
static void sort_range(int *a, int *tmp, size_t n) {
    if (n < 2) return;
    size_t mid = n / 2;
    sort_range(a, tmp, mid);
    sort_range(a + mid, tmp, n - mid);
    merge(a, mid, a + mid, n - mid, tmp);
    memcpy(a, tmp, n * sizeof *a);
}
```

Allocate once in the public function, not at every recursion level. If `malloc` fails, return `-1`.

## Common mistakes

- Taking from `b` on ties (`<=`), which breaks stability.
- Forgetting the leftover loops after the main merge.
- Splitting with `mid = n / 2` but then passing the wrong length for the right half.

Implement `merge`, `merge_sort`, and a stable sort for records.
