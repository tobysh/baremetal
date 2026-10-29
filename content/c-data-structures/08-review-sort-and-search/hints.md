`is_sorted` is a loop comparing `a[i - 1]` with `a[i]`. For the other two you need a sorted copy: write one helper that mallocs, copies and sorts.
===
Sort the copy with a small insertion sort (or `qsort` with a comparison function that avoids overflow by returning `(x > y) - (x < y)`). The median is `copy[(n - 1) / 2]`.
===
For the range count: `lower` = first index with `copy[i] >= lo`, `upper` = first index with `copy[i] > hi`, both by binary search; the answer is `upper - lower`. Handle `lo > hi` first, and `free(copy)` before returning.
