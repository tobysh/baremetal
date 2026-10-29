Complete these in the starter:

- `fn sum_chunks(chunks: Vec<Vec<u64>>) -> Vec<u64>`: spawn one thread per chunk that sums it, then return the sums in the same order as the chunks.
- `fn run_guarded(f: fn() -> i32) -> Result<i32, String>`: run `f` on a new thread. Return `Ok(value)`, or `Err(String::from("worker panicked"))` if the thread panicked.
- `fn on_new_thread(f: fn() -> u64) -> (u64, ThreadId)`: run `f` on a new thread and return its result together with that thread's `ThreadId` (taken inside the thread).
