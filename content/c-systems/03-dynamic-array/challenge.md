**Challenge: insert in the middle, and give memory back.**

The starter already has the lesson's `vec_init`, `vec_push`, `vec_pop` and `vec_free`. Add two more operations:

- `int vec_insert(IntVec *v, size_t i, int x)` puts `x` at index `i` and shifts everything from `i` onwards one slot to the right. `i` may be anything from `0` to `v->len` (inserting at `len` is a push). Grow exactly as `vec_push` does (capacity 4, then doubling). Return `0` on success. Return `-1` and leave the vector untouched if `i > len` or memory ran out.
- `int vec_remove_at(IntVec *v, size_t i, int *out)` removes the element at `i`, stores it in `*out` (when `out` is not `NULL`) and shifts the later elements one slot to the left. Return `-1` and change nothing if `i >= len`.

**Shrinking.** A vector that was once huge should not keep its memory forever. After a removal, if `cap > 4` and `len <= cap / 4`, `realloc` the buffer down to `cap / 2` (once per removal, never below 4). If that `realloc` fails, just keep the old buffer: shrinking is optional, losing data is not. Do not shrink when the vector is still more than a quarter full.

Use `memmove`, not `memcpy`, to shift: the source and destination overlap. The tests check the exact capacity after every step.
