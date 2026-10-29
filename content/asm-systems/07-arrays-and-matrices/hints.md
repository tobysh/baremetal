`prefix_sums`: keep the running total in `rax` and the index in `rcx`. Each iteration: `add rax, [rdi + rcx*8]` then `mov [rdi + rcx*8], rax`, `inc rcx`. The function returns nothing; `rax` is just a scratch register here.
===
`trace`: the stride in bytes is `(n + 1) * 4`: `lea r8, [rsi + 1]` and `shl r8, 2`. Loop `rcx` from 0 to `n`, with the pointer in `rdi`: `movsxd rdx, dword ptr [rdi]`, `add rax, rdx`, `add rdi, r8`.
===
Test `n` against zero before the first load: `test rsi, rsi` / `jz` to the exit. The 32-bit load must use `movsxd`, not `mov edx, [rdi]`, or negative diagonal values come out as huge positive numbers.
