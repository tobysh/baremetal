Implement `long weighted8(long a, long b, long c, long d, long e, long f, long g, long h)` in assembly. It returns

`1*a + 2*b + 3*c + 4*d + 5*e + 6*f + 7*g + 8*h`

The first six arguments arrive in `rdi`, `rsi`, `rdx`, `rcx`, `r8`, `r9`; `g` and `h` are on the stack. Return the result in `rax`. Arithmetic wraps around in 64 bits like C `long` (no overflow checks needed). The function must leave `rbx`, `rbp` and `r12`-`r15` unchanged, and `rsp` must be the same on return.
