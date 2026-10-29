Keep the running sum in `rax` (cleared with `xor eax, eax`), `rdi` as the read cursor and `rsi` as the number of bytes left.
===
Main loop: while `rsi >= 2`: `movzx edx, word ptr [rdi]`, `add rax, rdx`, `add rdi, 2`, `sub rsi, 2`. Then, if `rsi` is 1, `movzx edx, byte ptr [rdi]` / `add rax, rdx`.
===
Fold: repeat `mov rdx, rax` / `shr rdx, 16` / `jz done` / `movzx eax, ax` / `add rax, rdx`. Finish with `not eax` and `movzx eax, ax`.
===
An unaligned `movzx edx, word ptr [rdi]` is fine on x86; there is nothing to handle.
