Layout: `.section .rodata`, `.align 8`, `.Ltable: .quad .Ladd, .Lsub, .Lmul, .Land, .Lor, .Lxor, .Lshl, .Lmax`, then `.text`. Dispatch: `cmp rdi, 7` / `ja .Ldefault` / `jmp qword ptr [.Ltable + rdi*8]`.
===
Each case computes into `rax` from `rsi` and `rdx`, then `ret`: for example `.Ladd: lea rax, [rsi + rdx]` / `ret`. Shift: `mov rax, rsi` / `mov rcx, rdx` / `shl rax, cl` / `ret`. Max: `mov rax, rsi` / `cmp rax, rdx` / `cmovl rax, rdx` / `ret`.
===
`op_name`: define eight strings with `.asciz` plus `"?"`, and a table of eight `.quad` pointers to them. `cmp rdi, 7` / `ja` returns `lea rax, [rip + .Lq]`; otherwise `lea rax, [rip + .Lnames]` / `mov rax, [rax + rdi*8]` / `ret`.
