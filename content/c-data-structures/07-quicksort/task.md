Implement:

- `size_t partition(int *a, size_t n)` (with `n >= 1`) uses `a[n - 1]` as pivot and rearranges `a` so that every element left of the returned index is `<= pivot`, the element at the returned index **is** the pivot, and every element right of it is `>= pivot`. Return the pivot's final index. The array must keep exactly the same elements.
- `void quicksort(int *a, size_t n)` sorts ascending in place, using `partition`. It must survive sorted and all-equal inputs of a few thousand elements (see the lesson on recursing into the smaller side).
- `int quickselect(int *a, size_t n, size_t k)` returns the k-th smallest element (`k = 0` is the minimum), `0 <= k < n`. It may reorder `a`, but must not sort the whole array.
