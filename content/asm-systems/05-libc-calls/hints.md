`format_pair`: after the argument shuffle (`mov r8, rcx` / `mov rcx, rdx` / `lea rdx, [rip + .Lfmt]`), execute `sub rsp, 8`, `xor eax, eax`, `call snprintf`, `add rsp, 8`, `ret`. The `sub` makes the stack aligned.
===
Define the format string before use: `.section .rodata`, `.Lfmt: .asciz "%ld,%ld"`, then `.text` again. Without the `.text` the following code would be placed in read-only data.
===
`joined_length`: `sub rsp, 24`, `mov [rsp], rsi`, `call strlen` (a is already in `rdi`), `mov [rsp + 8], rax`, `mov rdi, [rsp]`, `call strlen`, `add rax, [rsp + 8]`, `add rsp, 24`, `ret`.
