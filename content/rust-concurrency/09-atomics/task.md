Complete these in the starter:

- `fn count_atomic(threads: usize, per: usize) -> usize`: `threads` threads each increment one shared `AtomicUsize` `per` times (no mutex). Return the total.
- `fn next_id() -> u64`: return a process-wide unique id using the `static NEXT_ID: AtomicU64`. Ids must never repeat, even when many threads call it at once.
- `fn record_max(cur: &AtomicI64, v: i64)`: raise `cur` to `v` if `v` is larger, atomically (hint: `fetch_max`).
- `Stop` is a shared flag: `new()`, `request(&self)` and `is_requested(&self)`. Then `fn spawn_counter(stop: Arc<Stop>) -> JoinHandle<u64>` starts a thread that counts loop rounds (at least one round) until the flag is set, and returns the number of rounds.
