Implement two functions:

- `unsigned long gcd_rec(unsigned long a, unsigned long b)` returns the greatest common divisor, computed recursively by Euclid's algorithm. `gcd(a, 0) = a`, and `gcd(0, 0) = 0`.
- `size_t count_char(const char *s, int c)` returns how many times the byte `c` occurs in the string `s`. It must use `strchr`. If `c` is 0 return 0.

Standard argument registers; results in `rax`. Keep the stack aligned at each call and preserve `rbx`, `rbp` and `r12`-`r15`.
