`fact_rec`: if `rdi <= 1` (unsigned), return 1. Otherwise `push rdi`, `dec rdi`, `call fact_rec`, `pop rdi`, `imul rax, rdi`, `ret`. The single push also fixes the alignment.
===
`fib_rec`: if `rdi < 2` (`jb`), return `rdi` itself (`mov rax, rdi` / `ret`). Otherwise `sub rsp, 24`, store `n` at `[rsp]`, call with `rdi - 1`, store the result at `[rsp + 8]`.
===
Second half of `fib_rec`: `mov rdi, [rsp]`, `sub rdi, 2`, `call fib_rec`, `add rax, [rsp + 8]`, `add rsp, 24`, `ret`.
