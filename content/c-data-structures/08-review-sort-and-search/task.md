Implement:

- `int is_sorted(const int *a, int n)` returns 1 if `a` is in non-decreasing order, else 0. Arrays with 0 or 1 elements are sorted.
- `int median_of(const int *a, int n)` (`n >= 1`) returns the lower median: the element at index `(n - 1) / 2` of the sorted array. For an even count, that is the smaller of the two middle values. The input array must not be changed.
- `int count_in_range(const int *a, int n, int lo, int hi)` returns how many elements `x` satisfy `lo <= x <= hi`, or `-1` if memory runs out. If `lo > hi` the answer is 0. The input array must not be changed, and the array is **not** assumed to be sorted.

For `count_in_range`, use a sorted copy and two binary searches rather than a linear count.
