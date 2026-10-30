**Challenge: insert and remove in the middle.**

Using the `IntVec` from the lesson (the tests start with `data == NULL`, `len == 0`, `cap == 0`), write two functions:

- `int vec_insert(IntVec *v, size_t i, int value)` puts `value` at index `i` and shifts everything from `i` onwards one place up. `i == len` appends. If `i > len`, return -1 and change nothing. When `len == cap`, grow first: capacity 4 if it is 0, otherwise double it. Return 0 on success; on allocation failure return -1 and leave the vector valid.
- `int vec_remove(IntVec *v, size_t i, int *out)` removes the item at index `i` and closes the gap. If `out` is not `NULL`, store the removed value in `*out`. If `i >= len`, return -1 and change nothing. Return 0 on success.

**Shrink as you go.** After a removal, if `len * 4 <= cap`, halve the capacity with `realloc` (but never below 4). If that `realloc` fails, keep the old block and still return 0.

Hints: `memmove` handles overlapping ranges, which `memcpy` does not. Mind the byte counts: `(len - i) * sizeof(int)`.
