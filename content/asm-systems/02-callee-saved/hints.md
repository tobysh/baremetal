Push `rbx`, `r12`, `r13`, `r14`, `r15` first (five pushes: the stack is then aligned). Copy `rdi` to `rbx`, `rsi` to `r12`, `rdx` to `r15`, and zero `r13` (index) and `r14` (count) with `xor r13d, r13d`.
===
Loop: `cmp r13, r12` / `jae .Ldone`; `mov rdi, [rbx + r13*8]`; `call r15`; `test rax, rax`; `jz .Lnext`; `inc r14`; `.Lnext: inc r13`; `jmp .Lloop`.
===
At `.Ldone` do `mov rax, r14`, then pop in reverse order: `pop r15`, `pop r14`, `pop r13`, `pop r12`, `pop rbx`, and `ret`. If `n` is 0 the loop body never runs, and the same epilogue still executes.
