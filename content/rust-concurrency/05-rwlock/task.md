Complete these in the starter:

- `Registry` holds `Arc<RwLock<HashMap<String, u32>>>`. `Registry::new()`; `handle(&self)` returns another `Registry` sharing the same map.
- `get(&self, key: &str) -> Option<u32>` reads under a read lock. `set(&self, key: &str, value: u32)` overwrites under a write lock.
- `incr(&self, key: &str) -> u32` adds 1 (starting from 0 for a new key) as one atomic step and returns the new value.
- `snapshot(&self) -> Vec<(String, u32)>` returns all entries sorted by key.
- `fn can_read_now(lock: &RwLock<i32>) -> bool`: `true` if a read lock could be taken right now without blocking.
