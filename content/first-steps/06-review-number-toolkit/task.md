Write three functions:

- `int digit_sum(int n)` returns the sum of the decimal digits of `n`. `n` is never negative. `digit_sum(1234)` is `10`, and `digit_sum(0)` is `0`.
- `int count_divisible(int lo, int hi, int d)` returns how many whole numbers from `lo` to `hi`, **including both ends**, are divisible by `d`. `d` is always greater than 0. If `lo` is greater than `hi` the range is empty, so the answer is `0`.
- `int is_prime(int n)` returns `1` if `n` is prime and `0` otherwise. Numbers below 2 are not prime.
