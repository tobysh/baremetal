Implement three functions:

- `double distance(double x1, double y1, double x2, double y2)` returns the Euclidean distance between the two points. Use `sqrtsd`.
- `double mean(const long *a, size_t n)` returns the arithmetic mean of `n` signed 64-bit integers as a double: convert each element and accumulate in a double, then divide by `n`. For `n = 0` return `0.0`.
- `long floor_to_long(double x)` returns the largest integer not greater than `x` (so `-2.5` gives `-3`). Values are within the `long` range.

Floating-point arguments and results use `xmm0`-`xmm3`; integer ones use `rdi` and `rsi`. Preserve `rbx`, `rbp` and `r12`-`r15`.
