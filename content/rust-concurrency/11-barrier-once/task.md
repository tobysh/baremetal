Complete these in the starter:

- `fn lockstep(n: usize, rounds: usize) -> Vec<Vec<usize>>`: spawn `n` threads sharing a counter (an `Arc<Mutex<usize>>`) and a `Barrier` of size `n`. In each round every thread adds 1 to the counter, waits at the barrier, then reads the counter. Return, for each thread, the values it read per round. Because of the barrier every value in round `r` (0-based) is `n * (r + 1)`. Mind that a fast thread must not start the next round's increment before a slow thread has read this round's value.
- `fn leader_count(n: usize, rounds: usize) -> usize`: spawn `n` threads sharing a `Barrier`; each calls `wait()` `rounds` times. Return how many `wait()` results said `is_leader()` in total (exactly one per round).
- `fn ensure_init()`: increments the static `INIT_RUNS` exactly once in the life of the process, however often and from however many threads it is called, using `Once`.
- `fn config() -> &'static String`: build the string `"config-v1"` on first use via the static `CONFIG` `OnceLock`, bumping `CONFIG_BUILDS` when (and only when) it builds it.
