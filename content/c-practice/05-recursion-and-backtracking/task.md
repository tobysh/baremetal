Implement these three functions using recursion. Helper functions are fine.

- `int sum_digits(int n)` returns the sum of the decimal digits of `n`, which will be `0` or more. `sum_digits(472)` is `13`, and `sum_digits(0)` is `0`. Use recursion, not a loop.
- `int count_subsets(const int *a, int n, int target)` returns how many subsets of the `n` numbers in `a` add up to `target`. Each position is a separate item, so duplicates count separately: in `{1, 1}` there are two subsets that sum to `1`. The empty subset has sum `0`, so `count_subsets(a, n, 0)` counts it. `n` will be between `0` and `16`, and the numbers are small positive or negative integers.
- `int count_queens(int n)` returns how many ways `n` queens can be placed on an `n` by `n` board with no two attacking each other. `n` will be between `1` and `8`. The answers for `n = 1` to `8` are `1, 0, 0, 2, 10, 4, 40, 92`.
