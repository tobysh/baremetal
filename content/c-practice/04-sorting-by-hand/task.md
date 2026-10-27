Implement these in the starter (include `<stdlib.h>` for `malloc` and `free`):

- `void insertion_sort(int *a, int n)` sorts `a` in ascending order, in place, using insertion sort. `n` may be `0` or `1`.
- `int merge_sorted(const int *a, int na, const int *b, int nb, int *out)` merges the sorted array `a` (length `na`) and sorted array `b` (length `nb`) into `out`, which has room for `na + nb` items. Returns `na + nb`. Either length may be `0`. Equal items are taken from `a` first.
- `int merge_sort(int *a, int n)` sorts `a` in ascending order using merge sort (split, sort both halves, merge). It may allocate a temporary buffer with `malloc` and must `free` it. Returns `0` on success, or `-1` if the allocation fails. `n` may be `0` or `1`, and duplicates and negative numbers must work.
