Complete these in the starter:

- `impl Iterator for Fib` with `type Item = u64`. The sequence is 0, 1, 1, 2, 3, 5, 8, ... `fn fib() -> Fib` returns a fresh one. Stop (return `None`) instead of overflowing: use `checked_add`.
- `impl Iterator for Countdown` with `type Item = u32`. `fn countdown(from: u32) -> Countdown` yields `from, from - 1, ..., 1` and then `None`. `countdown(0)` yields nothing.
- `fn sum_even_fibs_below(limit: u64) -> u64` adds up the even Fibonacci numbers that are strictly less than `limit`. Build it from `fib()` and adaptors, not a hand-written loop. For `limit = 0` the answer is `0`.
