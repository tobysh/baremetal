`Entry`, `HashTable`, `BUCKETS` and `hash_str` are in the starter. Implement:

- `void ht_init(HashTable *t)` sets every bucket to `NULL`.
- `int ht_put(HashTable *t, const char *key, int value)` stores the pair. If `key` already exists, replace its value (do not add a second entry). Otherwise add a new entry holding its **own heap copy** of the key. Return `0` on success, `-1` if memory ran out.
- `int ht_get(const HashTable *t, const char *key, int *out)` returns `1` and stores the value in `*out` if the key exists, otherwise returns `0` and leaves `*out` alone.
- `void ht_free(HashTable *t)` frees every entry and every key copy, and leaves the table empty (all buckets `NULL`).

Pick the bucket with `hash_str(key) % BUCKETS`. Use `strcmp` to compare keys. Do not use `strdup`.
