The starter defines `Store`, `Entry`, the callback type and the codes (`OK 0`, `NULL_ARG -1`, `BAD_KEY -2`, `OVERFLOW -3`, `NOT_FOUND -4`). Implement:

- `kv_new() -> *mut Store`, `unsafe kv_free(s)` (null is a no-op; create stores with `Store::new`).
- `unsafe kv_add(s, key, key_len, delta, out) -> i32`: add `delta` to the key's value (a missing key counts as 0 and is created). Null `s`, or null `key` with `key_len > 0`, is `NULL_ARG`; `key_len == 0` is `BAD_KEY`; an addition that overflows `i64` is `OVERFLOW` and changes nothing (a missing key is not created either). `out` may be null; if not, it receives the new value on `OK` only.
- `unsafe kv_get(s, key, key_len, out) -> i32`: same argument checks, `NOT_FOUND` for a missing key, `out` only written on `OK` (and may be null).
- `unsafe kv_remove(s, key, key_len) -> i32`: `OK` if removed, `NOT_FOUND` if absent, same argument checks.
- `unsafe kv_each(s, cb, ctx) -> isize`: call `cb(ctx, &entry)` per key in ascending byte order; a non-zero return stops iteration. Return the number of callbacks made, or -1 for a null `s`.

Different key sets, deltas near the `i64` limits and early stops are tried.
