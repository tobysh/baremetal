The types are given. A cache has a capacity `cap >= 1` and stores int keys and int values.

- `Lru *lru_new(size_t cap)` creates an empty cache with `cap >= 1` (return `NULL` for `cap == 0` or on allocation failure). Use `nb = 2 * cap + 1` buckets, bucket = `(unsigned)key % nb`.
- `int lru_get(Lru *c, int key, int *out)` returns `0` and stores the value if the key is present, making that entry the most recently used. Returns `-1` on a miss.
- `void lru_put(Lru *c, int key, int val)` inserts or updates the key and makes it the most recently used. If the cache then holds more than `cap` entries, remove the least recently used one.
- `size_t lru_size(const Lru *c)` returns the number of entries.
- `int lru_oldest(const Lru *c, int *key)` stores the key of the least recently used entry and returns `0`, or returns `-1` if the cache is empty. It does not change recency.
- `void lru_free(Lru *c)` frees everything (`NULL` is fine).

All operations except `lru_free` must take constant time on average: no scanning the whole list.
