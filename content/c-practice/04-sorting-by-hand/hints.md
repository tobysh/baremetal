Insertion sort: for each `i` from `1`, save `a[i]` in `key`, shift larger items on its left one place to the right, then write `key` into the gap.
===
`merge_sorted`: keep three positions `i` (in `a`), `j` (in `b`) and `k` (in `out`). While both have items left, copy the smaller one. Then copy whatever remains from whichever array is not finished.
===
`merge_sort`: if `n < 2` return `0`. Allocate `n` ints for a buffer. Write a helper `static void sort_rec(int *a, int n, int *tmp)` that recurses on both halves, calls `merge_sorted(a, mid, a + mid, n - mid, tmp)`, then copies `tmp` back into `a` with a loop. `free(tmp)` at the end.
