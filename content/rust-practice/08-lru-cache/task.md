`LruCache` (fields `capacity`, `map`, `order`) and `new(capacity: usize)` are given. Complete these methods:

- `get(&mut self, key: &str) -> Option<i32>` returns the value if present, and marks the key as the most recently used. A missing key returns `None` and changes nothing.
- `put(&mut self, key: String, value: i32) -> Option<String>` stores the value and marks the key as most recently used.
  - Updating an existing key replaces its value and refreshes it; nothing is evicted.
  - Adding a new key to a full cache first evicts the least recently used key, and returns `Some(evicted_key)`.
  - Otherwise returns `None`.
  - A cache with capacity `0` stores nothing: `put` returns `None` and the cache stays empty.
- `contains(&self, key: &str) -> bool` does **not** change the order.
- `len(&self) -> usize` is the number of stored keys.
- `keys_oldest_first(&self) -> Vec<String>` lists the keys from least to most recently used.

The map and the order list must always hold exactly the same keys.
