Implement these without conditional jumps (they are allowed, but the point is to use `cmov` and `set`):

- `long max3(long a, long b, long c)` returns the largest of three signed values.
- `int sign_of(long x)` returns -1, 0 or 1.
- `unsigned long sat_sub(unsigned long a, unsigned long b)` returns `a - b`, or 0 if `b > a` (saturating subtraction).

Inputs include negatives, ties and values near the 64-bit limits.
