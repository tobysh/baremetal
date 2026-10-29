Implement:

- `uint64_t divmod_u(uint64_t n, uint64_t d, uint64_t *rem)` returns `n / d` and stores `n % d` in `*rem`. `d` is never 0.
- `long floor_div(long a, long b)` returns `a / b` rounded toward negative infinity (so `floor_div(-7, 2) = -4`). `b` is never 0, and `LONG_MIN / -1` is not tested.
- `long mod_pos(long a, long m)` returns `a` modulo `m` in the range `[0, m)` for `m > 0` (so `mod_pos(-7, 5) = 3`).
