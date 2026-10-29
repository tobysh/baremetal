`add_checked`: `mov rax, rdi` / `add rax, rsi` / `jo` to the failure label. Only after the check do `mov [rdx], rax` and return 1.
===
`mul_checked_u`: the output pointer is in `rdx`, which `mul` destroys. First `mov r8, rdx`, then `mov rax, rdi` / `mul rsi` / `jo` failure, `mov [r8], rax`.
===
`fact_checked`: keep the accumulator in `rax` (start at 1) and the counter in `rcx`/`rsi`. For `i = 2..n` do `mul` by `i` (copy `i` to a register, since `mul` takes a register or memory operand, not an immediate) and `jo` on failure. Save `rdx`'s pointer in `r8` as before. 21! is the first that overflows.
===
Remember that `n` is an `unsigned` in `edi`; `mov ecx, edi` zero-extends it into `rcx`.
