The starter gives you `maxHeight`, the `node` and `SkipList` types and `NewSkipList(seed)`. Implement:

- `Put(key string, value []byte)` inserts the key or replaces its value (store a copy of `value`).
- `Get(key string) ([]byte, bool)`.
- `Delete(key string) bool` removes the key and reports whether it was present. Unlink it from every level.
- `Len() int`, the number of keys.
- `Seek(key string) (string, []byte, bool)` returns the first key that is **greater than or equal to** `key`, its value, and `true`; or `"", nil, false` when there is none.
- `Keys() []string` returns every key in ascending order by walking level 0 (an empty, non-nil slice when empty).
- `Height() int` returns the number of levels currently in use: 1 when empty, and never more than `maxHeight`. With thousands of keys it must be well above 1, because a one-level skip list is just a slow linked list.
- `randomHeight()` is a method you write: height 1 plus one level for each successful coin flip (`s.rng.Intn(2) == 0`), capped at `maxHeight`.

The tests compare your list with a plain map and sorted slice over thousands of random operations, and insert 100,000 ascending keys, which only finishes in time if searches really use the express lanes.
