The types and `oa_hash` are given. The table has a fixed capacity (a power of two) and never resizes. Slot index is `oa_hash(key) & (cap - 1)`, probing forward by one with wrap-around.

- `int oa_init(OAMap *m, size_t cap)` allocates `cap` slots, all `EMPTY`. Returns `0`, or `-1` if `cap < 2`, `cap` is not a power of two, or `calloc` fails (leave `slots` `NULL`).
- `int oa_put(OAMap *m, int key, int val)` inserts the pair, or updates the value if the key exists (then `count` is unchanged). Reuse a `DELETED` slot for a new key when possible. Returns `0`, or `-1` if the key is new and there is no free slot.
- `int oa_get(const OAMap *m, int key, int *out)` stores the value in `*out` and returns `0`, or returns `-1` if the key is absent. It must terminate even if the table has no `EMPTY` slot.
- `int oa_del(OAMap *m, int key)` marks the slot `DELETED` and returns `0`, or returns `-1` if the key is absent.
- `void oa_free(OAMap *m)` frees the slots and resets the map.

`count` is the number of live entries. Probe at most `cap` slots in any loop.
