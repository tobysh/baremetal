Write three functions that work with pointers into an array. Other arrays and other ranges are tried, including sub-ranges starting in the middle of an array.

- `int sum_between(const int *begin, const int *end)` returns the sum of the elements in the range `[begin, end)`, that is, `begin` is included and `end` is not. If `begin == end` the sum is 0.
- `const int *find_max(const int *arr, size_t n)` returns a pointer to the largest of the `n` elements (to the first one if several are equal), or `NULL` if `n` is 0.
- `ptrdiff_t gap(const int *a, const int *b)` returns how many elements you step over to get from `a` to `b` (negative if `b` is before `a`). Both point into the same array.
