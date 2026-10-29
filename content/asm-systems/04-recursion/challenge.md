**Challenge: two recursions that branch.**

Factorial and Fibonacci made one call (or two simple ones). These two need you to keep real state alive across calls.

- `unsigned long binom_rec(unsigned long n, unsigned long k)` returns the binomial coefficient "n choose k" using **Pascal's rule**: `C(n, k) = C(n-1, k-1) + C(n-1, k)`, with `C(n, 0) = C(n, n) = 1`. If `k > n` the answer is `0`. `n` in `rdi`, `k` in `rsi`. Do not multiply and divide factorials; the function has to call itself twice, so it must remember both the original `n` and `k` and the first result across the first call. The tests use `n` up to 22.
- `unsigned long ackermann(unsigned long m, unsigned long n)` is the classic function that grows terrifyingly fast: `A(0, n) = n + 1`, `A(m, 0) = A(m-1, 1)`, `A(m, n) = A(m-1, A(m, n-1))`. `m` in `rdi`, `n` in `rsi`. The tests use `m` up to 3 and `n` up to 8 (and `A(4, 0)`). Notice that some of the calls are the last thing the function does: a `jmp` instead of `call; ret` saves stack, but is optional.

`rsp` must be 16-byte aligned at every `call`, and `rbx`, `rbp` and `r12`-`r15` must come back unchanged.
