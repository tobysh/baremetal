Use `rax` for the running total, starting at zero (`xor eax, eax`), and a second register such as `rcx` as the index `i`.
===
The loop skeleton: a label, `cmp rcx, rsi`, `jae` to the exit, the body, `inc rcx`, `jmp` back to the label. Because the test is at the top, `n = 0` works for free.
===
The body is a single instruction: `add rax, [rdi + rcx*8]`. The `*8` is the size of one `long`.
