---
title: Sorting by Hand
summary: Write insertion sort and merge sort yourself, and learn why one is simple and slow while the other is clever and fast.
minutes: 35
difficulty: medium
id: c-practice/sorting-by-hand
mode: harness
---
`qsort` exists, but writing a sort by hand teaches you to think about arrays, loops and invariants. Two classic sorts show two very different ideas.

## Insertion sort: sorting a hand of cards

Pick up cards one at a time. Each new card, you slide into its correct place among the cards you are already holding. The cards in your hand are always sorted.

In code, the part of the array to the left of position `i` is the sorted "hand". Take `a[i]`, and shift every bigger item one step right to make a gap:

```c
for (int i = 1; i < n; i++) {
    int key = a[i];
    int j = i - 1;
    while (j >= 0 && a[j] > key) {
        a[j + 1] = a[j];     // shift right
        j--;
    }
    a[j + 1] = key;          // drop it in the gap
}
```

Careful: `j >= 0` must be checked **before** `a[j]`, otherwise you read before the start of the array. (`&&` stops at the first false part, so the order matters.)

Insertion sort is wonderful for small or nearly-sorted arrays. For `n` items it may do around `n * n` steps in the worst case, which gets painful at thousands of items.

## Merge sort: divide and conquer

Merge sort rests on a neat fact: **merging two already-sorted lists is easy.** Look at the front of each list, take the smaller, repeat.

```
left:  1 4 9        right: 2 3 10
out:   1 2 3 4 9 10
```

So to sort an array, split it in half, sort each half the same way (recursion!), then merge. A single item is already sorted, which is where the recursion stops.

```c
void sort(int *a, int n) {
    if (n < 2) return;
    int mid = n / 2;
    sort(a, mid);
    sort(a + mid, n - mid);
    // merge a[0..mid) and a[mid..n) using a temporary buffer
}
```

The merge needs somewhere to write, because merging in place would overwrite items you still need. Use `malloc` for a temporary buffer, merge into it, copy back, and `free` it. Merge sort needs only about `n * log n` steps, so a million items take a moment instead of a lifetime.

A merge taking the *left* item when two are equal keeps equal items in their original order. That property is called being **stable**.

## Your turn

Write the three pieces: `insertion_sort`, the standalone `merge_sorted` step that merges two sorted arrays into a third, and `merge_sort`, which uses it. Because merge sort allocates, it returns a status.
