Implement these functions for `IntVec` (the struct is given). All return `0` on success and `-1` on failure.

- `void vec_init(IntVec *v)` makes an empty vector: `data` is `NULL`, `len` and `cap` are 0.
- `int vec_push(IntVec *v, int x)` appends `x`. When `len == cap`, grow the capacity to 4 if it was 0, otherwise double it. Return `-1` if `realloc` fails.
- `int vec_get(const IntVec *v, size_t i, int *out)` stores element `i` in `*out`. Return `-1` (and leave `*out` alone) if `i >= len`.
- `int vec_pop(IntVec *v, int *out)` removes the last element and stores it in `*out`. Return `-1` if the vector is empty. Capacity does not shrink.
- `int vec_insert(IntVec *v, size_t i, int x)` inserts `x` so that it ends up at index `i`, shifting later elements right. `i == len` appends. Return `-1` if `i > len`, leaving the vector unchanged.
- `void vec_free(IntVec *v)` frees the block and resets the vector to the empty state, so calling `vec_free` twice is harmless.

The tests append hundreds of items and check that the capacity grows geometrically, not one slot at a time.
