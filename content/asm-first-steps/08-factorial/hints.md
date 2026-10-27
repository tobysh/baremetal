Base case first: `cmp rdi, 1`, `jbe` to a label that does `mov eax, 1` and `ret`. That handles both 0 and 1.
===
In the recursive case you need `n` after the call, but `rdi` will be overwritten. Save `n` in `rbx`. Since `rbx` is callee-saved, `push rbx` first and `pop rbx` before `ret`.
===
Sequence: `push rbx`, `mov rbx, rdi`, `lea rdi, [rdi - 1]`, `call factorial`, `imul rax, rbx`, `pop rbx`, `ret`. The single `push` also fixes the alignment: entry has `rsp` at 8 mod 16, so after one push it is a multiple of 16.
