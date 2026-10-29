The type `Rec` (a `key` and the original `order`) is given. Implement:

- `void merge(const int *a, size_t na, const int *b, size_t nb, int *out)` merges two sorted arrays into `out`, which has room for `na + nb` ints. Either input may be empty (its pointer may then be `NULL`). `out` does not overlap the inputs.
- `int merge_sort(int *a, size_t n)` sorts `a` ascending in place using one scratch buffer. Returns `0`, or `-1` if `malloc` fails. It must be O(n log n): the tests sort tens of thousands of values including reversed and all-equal arrays.
- `int merge_sort_recs(Rec *a, size_t n)` sorts records by `key` ascending and is **stable**: records with equal keys keep their original relative order. Same return values.

You may write a shared helper, but each public function must work on its own.
