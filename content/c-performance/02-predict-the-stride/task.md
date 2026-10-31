Implement both functions:

- `long predict_misses(long n, long stride)` returns the number of cache misses of `scan(n, stride)` started on a cold cache. `n` is in `0..1024`, `stride >= 1`. `n = 0` reads nothing.
- `int sets_touched(long stride_lines)` returns how many of the 16 sets are visited by an endless scan that advances `stride_lines` (>= 1) whole lines at a time starting at a line in set 0.

Do not change `scan` or the cache block. The prediction must be computed, not looked up: many `(n, stride)` pairs are tried, including strides larger than `n`, strides just below and above one line, and strides that are not powers of two. The "ground truth" is the simulated cache run separately by the tests.
