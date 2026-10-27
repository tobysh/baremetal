Implement `unsigned long factorial(unsigned long n)` in assembly, **recursively**: it must call itself. Return `n!` (with `0! = 1`). Inputs up to 20 are tested, so the result fits in 64 bits.

`n` arrives in `rdi`; return in `rax`. Keep `rsp` 16-byte aligned at every `call`, and restore any callee-saved register (`rbx`, `rbp`, `r12`-`r15`) you use. The tests check that they come back unchanged.
