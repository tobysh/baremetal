The types and `str_hash` (djb2) are given. A key's bucket is `str_hash(key) % nbuckets`.

- `int sm_init(StrMap *m, size_t nbuckets)` allocates `nbuckets` empty buckets (`calloc`). Returns `0`, or `-1` if `nbuckets == 0` or allocation fails.
- `int sm_put(StrMap *m, const char *key, int val)` inserts a pair or updates the value of an existing key (an update does not change `count`). The map stores its **own copy** of the key. New entries go at the head of their chain. After inserting a new key, if `count > 2 * nbuckets`, double `nbuckets` and rehash every entry. Returns `0`, or `-1` on allocation failure.
- `int sm_get(const StrMap *m, const char *key, int *out)` returns `0` and stores the value, or `-1` if the key is absent.
- `int sm_del(StrMap *m, const char *key)` removes the entry and frees its key and node. Returns `0`, or `-1` if absent.
- `void sm_free(StrMap *m)` frees everything and resets the map.

The empty string is a valid key.
