Prologue: `push rbp`, `mov rbp, rsp`, `sub rsp, 32`. The scratch buffer is `[rbp-32, rbp)`. Keep a write cursor in `r8` starting at `rbp` (one past the end), and remember the caller's `buf` start for the length (`mov r9, rdi`).
===
Digit loop: `mov rax, rsi`, `mov rcx, 10`, then `xor edx, edx` / `div rcx` / `add dl, 48` / `dec r8` / `mov [r8], dl` / `test rax, rax` / `jnz` back. It is a do-while: `0` must still produce one digit.
===
Copy loop: while `r8 < rbp`: `mov dl, [r8]`, `mov [rdi], dl`, `inc r8`, `inc rdi`. Then `mov byte ptr [rdi], 0`, `mov rax, rdi`, `sub rax, r9` (the length), `leave`, `ret`.
