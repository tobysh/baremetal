`divmod_u`: the pointer arrives in `rdx`, which `div` overwrites. Copy it first with `mov r8, rdx`. Then `mov rax, rdi` / `xor edx, edx` / `div rsi` / `mov [r8], rdx` / `ret`.
===
`floor_div`: `mov rax, rdi` / `cqo` / `idiv rsi`. Now `rax` is the truncated quotient and `rdx` the remainder.
===
`floor_div` continued: if `rdx` is zero you are done. Otherwise `xor rdx, rsi` has its sign bit set exactly when the remainder and the divisor have opposite signs; in that case `dec rax`.
===
`mod_pos`: same `cqo` / `idiv`, result in `rdx`. If it is negative (`test rdx, rdx` / `jns`), add `m` (`add rdx, rsi`). Return it with `mov rax, rdx`.
