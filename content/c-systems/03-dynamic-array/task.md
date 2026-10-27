`IntVec` is defined in the starter. Implement:

- `void vec_init(IntVec *v)` makes an empty vector: `data` is `NULL`, `len` and `cap` are `0`. No allocation yet.
- `int vec_push(IntVec *v, int x)` appends `x`. When full, grow with `realloc`: start at a capacity of 4, then double. Return `0` on success, `-1` if memory ran out (leaving the vector unchanged).
- `int vec_pop(IntVec *v, int *out)` removes the last element and stores it in `*out`. Return `0` on success, or `-1` if the vector is empty (then leave `*out` alone).
- `void vec_free(IntVec *v)` frees the memory and resets the vector to the empty state, so calling it twice is safe.

Keep `cap >= len` always, and do not let `cap` grow beyond twice what you need.
