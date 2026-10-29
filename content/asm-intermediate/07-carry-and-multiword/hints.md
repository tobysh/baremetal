`add128`: `mov rax, [rdi]` / `add rax, [rsi]` / `mov rcx, [rdi + 8]` / `adc rcx, [rsi + 8]`, and only then store both limbs to `rdx`. Loading both before storing keeps it correct when `out` aliases an input.
===
`mulhi`: `mov rax, rdi` / `mul rsi` / `mov rax, rdx` / `ret`.
===
`add_n`: handle `n = 0` first (`xor eax, eax` / `test rcx, rcx` / `jz` out; `test` also clears CF). Use `r8` as the index and the counter in `rcx`.
===
Inside the loop use only `mov`, `adc`, `lea r8, [r8 + 1]` and `dec rcx` / `jnz`. Anything else (`inc` is fine, `add r8, 1` is not) corrupts the carry. After the loop `setc r9b` / `movzx eax, r9b`.
