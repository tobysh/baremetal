---
title: Quicksort and Quickselect
summary: Partition around a pivot and recurse. Write the Lomuto partition, quicksort, and quickselect, which finds the k-th smallest value without fully sorting.
minutes: 40
difficulty: hard
tags: sorting, recursion, algorithms
id: c-data-structures/quicksort
mode: harness
---
Merge sort does its work *after* the recursion (merging). **Quicksort** does its work *before*: it picks one element, the **pivot**, and rearranges the array so everything smaller is on its left and everything larger is on its right. The pivot is then in its final sorted position. Recurse on each side and you are done, with no scratch buffer.

## Lomuto partition

Take the last element as pivot. Keep a boundary `i` such that everything before it is `<= pivot`. Scan with `j`; whenever `a[j] <= pivot`, swap it to the boundary and advance the boundary. At the end swap the pivot into the boundary position.

```c
int pivot = a[n - 1];
size_t i = 0;
for (size_t j = 0; j + 1 < n; j++) {
    if (a[j] <= pivot) {
        swap(&a[i], &a[j]);
        i++;
    }
}
swap(&a[i], &a[n - 1]);
return i;      // the pivot's final index
```

After this, `a[0..i)` are `<= pivot`, `a[i]` is the pivot, and `a[i+1..n)` are `>= pivot` (in fact `> pivot`).

## Quicksort

```c
void quicksort(int *a, size_t n) {
    if (n < 2) return;
    size_t p = partition(a, n);
    quicksort(a, p);
    quicksort(a + p + 1, n - p - 1);
}
```

Average cost is O(n log n). The danger: with the last element as pivot, an already sorted array splits into pieces of size n-1 and 0 every time, giving O(n²) time and recursion n deep. Production sorts pick the pivot more carefully (median of three, random). To stay safe from deep recursion, recurse into the **smaller** side and loop on the larger:

```c
while (n >= 2) {
    size_t p = partition(a, n);
    if (p < n - p - 1) { quicksort(a, p); a += p + 1; n -= p + 1; }
    else               { quicksort(a + p + 1, n - p - 1); n = p; }
}
```

That keeps the stack depth at O(log n) whatever the input. The tests use sorted and all-equal arrays of a few thousand items.

## Quickselect

To find the k-th smallest (0-based) you do not need a full sort. Partition once: if the pivot landed at index `k`, you are done; if `k` is smaller, only the left side matters; otherwise only the right side does. You discard half the work each step and the average cost is O(n).

## Common mistakes

- `size_t` underflow: `p - 1` when `p == 0` wraps to a huge number. Pass lengths (`p`, `n - p - 1`) instead of end indices.
- Using `<` instead of `<=` in the partition test, which behaves badly with many equal values.
- Forgetting the final swap that puts the pivot in place.

Implement `partition`, `quicksort` and `quickselect`.
