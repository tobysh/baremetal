`total_score`: advance `rdi` by 16 per element and count `rcx` up to `rsi`. Body: `mov edx, [rdi]`, `test edx, edx`, `jle .Lskip`, then `movsxd rdx, edx`, `imul rdx, [rdi + 8]`, `add rax, rdx`, `movsxd rdx, dword ptr [rdi + 4]`, `add rax, rdx`.
===
`heaviest`: handle `n == 0` first (`mov rax, -1`). Keep `best` in `rax` and the best weight in `r8d` (`mov r8d, [rdi]`). Loop `rcx` from 1: the item is at `rdi + rcx*16`, which a scale cannot express, so compute `mov rdx, rcx` / `shl rdx, 4`, then use `[rdi + rdx]`.
===
In `heaviest`, compare with `mov r9d, [rdi + rdx]` / `cmp r9d, r8d` / `jle .Lnext` (signed, strictly greater needed). When greater: `mov r8d, r9d` and `mov rax, rcx`. At the end `ret` with `rax` = best.
