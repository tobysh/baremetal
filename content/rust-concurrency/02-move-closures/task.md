Complete these in the starter:

- `fn greet_all(names: Vec<String>) -> Vec<String>`: move each name into its own thread, which builds `"hello, <name>"`. Return the greetings in the order of `names`.
- `fn indexed_squares(n: usize) -> Vec<usize>`: spawn `n` threads; thread `i` returns `i * i`. Return the results in index order.
- `fn spawn_range_sum(start: u32, len: u32) -> JoinHandle<u32>`: start a thread that sums `start, start + 1, ... start + len - 1` and return its handle without joining it.
