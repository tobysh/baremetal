`isort` outer loop: `mov r8d, 1`; while `r8 < rsi` (unsigned compare is fine, both are sizes) load `key` with `mov rax, [rdi + r8*8]` and set `j`: `mov rcx, r8`.
===
Inner loop: stop if `rcx == 0`. Otherwise `mov rdx, [rdi + rcx*8 - 8]`; if `rdx <= rax` (signed: `cmp rdx, rax` / `jle`), stop. Else `mov [rdi + rcx*8], rdx`, `dec rcx` and repeat.
===
After the inner loop, `mov [rdi + rcx*8], rax`, `inc r8`, and go back to the outer test. For `n` of 0 or 1 the outer test fails immediately.
===
`count_inversions`: `rax` = total, `r8` = i, `r9` = j = i + 1. For each pair: `xor ecx, ecx` / `cmp rdx, [rdi + r9*8]` (with `rdx` = `a[i]`) / `setg cl` / `add rax, rcx`.
