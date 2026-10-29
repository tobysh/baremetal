Implement two **recursive** functions:

- `unsigned long fact_rec(unsigned long n)` returns `n!` (`0! = 1`). Results are taken modulo 2^64, as in C.
- `unsigned long fib_rec(unsigned long n)` returns the n-th Fibonacci number with `fib(0) = 0`, `fib(1) = 1`, `fib(n) = fib(n-1) + fib(n-2)`.

The argument is in `rdi`, the result in `rax`. Both must call themselves (the tests use `n` up to 25 for Fibonacci and 20 for factorial). Keep `rsp` aligned at every `call` and preserve `rbx`, `rbp` and `r12`-`r15`.
