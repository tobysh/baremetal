Build `Store`, an in-memory key-value store from string keys to `[]byte` values.

- `NewStore() *Store` returns an empty store.
- `Put(key string, value []byte)` stores a **copy** of `value`, replacing any old value.
- `Get(key string) ([]byte, bool)` returns a **copy** of the value and `true`, or `nil, false` when the key is missing. A key stored with an empty value is present.
- `Delete(key string) bool` removes the key and reports whether it existed.
- `Len() int` is the number of keys.
- `Keys() []string` returns all keys in ascending order (an empty, non-nil slice for an empty store).
