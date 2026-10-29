Start with `vec_init` and `vec_get`, then `vec_push`. Everything else builds on a helper that makes sure there is room for one more element.
===
A helper like `static int reserve_one(IntVec *v)` can do: if `v->len == v->cap`, compute `new_cap` (4 when `cap` is 0, else `cap * 2`), call `realloc` into a temporary pointer, and only on success store it in `v->data` and update `v->cap`.
===
`vec_push` is `reserve_one` followed by `v->data[v->len++] = x;`. `vec_insert` is `reserve_one`, then `memmove(&v->data[i + 1], &v->data[i], (v->len - i) * sizeof v->data[0])`, then store `x` and increase `len`. Check `i > v->len` before doing anything.
