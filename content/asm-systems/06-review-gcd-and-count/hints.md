`gcd_rec`: `test rsi, rsi` / `jnz .Lstep`; else `mov rax, rdi` / `ret`. At `.Lstep`: `mov rax, rdi`, `xor edx, edx`, `div rsi`, then `mov rdi, rsi`, `mov rsi, rdx` and either `jmp gcd_rec` (a tail call) or `sub rsp, 8` / `call gcd_rec` / `add rsp, 8` / `ret`.
===
`count_char`: `test esi, esi` first and return 0 if zero. Then push `rbx`, `r12`, `r13` (three pushes align the stack). Keep the cursor in `rbx`, `c` in `r12d`, the count in `r13`.
===
Loop: `mov rdi, rbx`, `mov esi, r12d`, `call strchr`, `test rax, rax`, `jz .Ldone`, `inc r13`, `lea rbx, [rax + 1]`, `jmp` back to the loop. At the end move `r13` to `rax` and pop in reverse. Make sure the early `c == 0` return happens before any push, or pops on that path as well.
