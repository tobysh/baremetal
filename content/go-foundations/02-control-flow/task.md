Implement these functions. Your code is tried with many different inputs.

- `SumTo(n int) int` returns `1 + 2 + ... + n`, or `0` when `n` is zero or negative.
- `Collatz(n int) int` counts the steps to reach `1` from `n` (`n >= 1`): while `n` is not 1, halve it if it is even, otherwise replace it with `3n + 1`. `Collatz(1)` is `0`; `Collatz(6)` is `8`.
- `DayKind(day string) string` returns `"weekend"` for `"sat"` and `"sun"`, `"weekday"` for `"mon"` to `"fri"`, and `"unknown"` for anything else. Use a `switch`.
- `IndexOf(xs []int, target int) int` returns the index of the first `target` in `xs`, or `-1` if it is absent.
- `SumEvensUntil(xs []int, limit int) int` adds up the even numbers of `xs` in order, skipping odd ones with `continue`, and stops at the first number greater than `limit` (that number is not added).
