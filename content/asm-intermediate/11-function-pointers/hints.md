`map_inplace`: three pushes (`rbx`, `r12`, `r13`) both save registers and align the stack. Copy `rdi`, `rsi` and `rdx` into them, loop while the count is non-zero, `call r13`, and store `rax` back through the cursor.
===
`fold` has four values to keep across the call: cursor, remaining count, `f` and the accumulator. That is four callee-saved registers, so push `rbx`, `r12`, `r13`, `r14` and then `sub rsp, 8` to fix the alignment (four pushes leave `rsp` at 8 mod 16). Undo it with `add rsp, 8` before the pops.
===
`fold` loop: `mov rdi, r14` (acc), `mov rsi, [rbx]` (element), `call r13`, `mov r14, rax`, advance the cursor, decrement the count. At the end `mov rax, r14`.
===
Move `rcx` (the function pointer) into a callee-saved register **at the start**, before anything else can overwrite it. The initial accumulator arrives in `rdx`.
