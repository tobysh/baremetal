`a` is sorted in ascending order (duplicates allowed) and has `n` elements. `n` may be 0, in which case `a` may be `NULL`. Implement:

- `int lower_bound(const int *a, int n, int x)` returns the first index `i` with `a[i] >= x`, or `n` if there is none.
- `int upper_bound(const int *a, int n, int x)` returns the first index `i` with `a[i] > x`, or `n` if there is none.
- `int count_equal(const int *a, int n, int x)` returns how many elements equal `x`, using the two functions above (no linear scan).

Do not modify the array.
