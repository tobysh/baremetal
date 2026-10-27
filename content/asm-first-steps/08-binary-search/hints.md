Use a half-open range: `lo = 0` in `rcx`, `hi = n` in `r8` (copy it from `rsi`). Loop while `lo < hi`: `cmp rcx, r8` then `jae` to the "not found" exit. An empty array exits immediately.
===
Midpoint: copy `hi` to `rax`, `sub rax, rcx`, `shr rax, 1`, `add rax, rcx`. Then load `r9` with `[rdi + rax*8]` and `cmp r9, rdx`.
===
If equal, `ret` with the index already in `rax`. If `a[mid] < key` (signed `jl`), set `lo = mid + 1` with `lea rcx, [rax + 1]`; otherwise set `hi = mid` with `mov r8, rax`. Then `jmp` to the loop top. The not-found exit is `mov rax, -1` and `ret`.
