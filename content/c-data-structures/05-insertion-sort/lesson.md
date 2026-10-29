---
title: Insertion Sort
summary: Sort an array the way you sort a hand of playing cards. Write insertion sort for ints and for strings, and count the shifts it needs.
minutes: 20
difficulty: easy
tags: sorting, arrays, algorithms
id: c-data-structures/insertion-sort
mode: harness
---
Picture sorting playing cards in your hand. You pick up one card at a time and slide it left past every bigger card until it fits. **Insertion sort** does exactly that on an array.

The invariant: before step `i`, the first `i` elements are already sorted. Step `i` takes `a[i]` (the "key") and inserts it into that sorted prefix.

```c
for (int i = 1; i < n; i++) {
    int key = a[i];
    int j = i - 1;
    while (j >= 0 && a[j] > key) {
        a[j + 1] = a[j];   // shift the bigger element right
        j--;
    }
    a[j + 1] = key;        // drop the key into the gap
}
```

Note the loop test order: check `j >= 0` **before** reading `a[j]`, or you read before the start of the array.

## Cost

In the worst case (reverse order) every element slides all the way left: about n²/2 shifts. On an already sorted array nothing shifts at all and the sort is O(n). That makes insertion sort a good choice for small or nearly sorted arrays, and real libraries use it to finish off small pieces inside faster sorts.

The number of shifts has a name: the **inversion count**, the number of pairs `i < j` with `a[i] > a[j]`. It measures how unsorted an array is. Each shift fixes exactly one inversion.

## Strings too

Sorting strings is the same algorithm. Compare with `strcmp(x, y) > 0` instead of `>`, and move `const char *` pointers around instead of copying characters. Sorting an array of pointers is cheap because only the pointers move.

## Stability

Insertion sort is **stable**: equal elements keep their original order, as long as you shift only on a strict `>`. Using `>=` would swap equal items and lose stability.

Implement both versions.
