Complete these in the starter:

- `fn tally_on_threads(n: usize) -> (usize, usize)`: spawn `n` threads. Each thread adds 1 to a shared total and records its own `ThreadId` in a shared set. Return `(total, distinct_thread_ids)`. For `n` threads that is `(n, n)`, and none of the ids may be the caller's. The starter's `Rc<RefCell<..>>` design cannot cross threads: switch to `Arc` and `Mutex`.
- `fn round_trip<T: Send + 'static>(value: T) -> (T, ThreadId)`: move `value` to a new thread, return it back together with the id of the thread that held it.
