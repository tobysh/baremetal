The `IntVec` struct is already defined in the starter. Write three functions. The tests push different numbers of values and check the contents each time.

- `int *new_zeroed(size_t n)` returns a heap array of `n` ints, all zero (use `calloc`). Return `NULL` if allocation fails.
- `int vec_push(IntVec *v, int value)` appends `value`. If `len == cap`, first grow the block with `realloc`: the new capacity is 4 if `cap` is 0, otherwise double the old one. Return 0 on success. On allocation failure return -1 and leave the vector unchanged and valid.
- `int vec_shrink(IntVec *v)` reduces the capacity to exactly `len` using `realloc`. If `len` is 0, free the block and set `data` to `NULL` and `cap` to 0. Return 0 on success (if the shrinking `realloc` fails, keep the old block and still return 0).
