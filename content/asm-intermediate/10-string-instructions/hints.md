`fill_bytes`: `rdi` is already the destination. Save it for the return value (`mov r8, rdi`), then `mov eax, esi` / `mov rcx, rdx` / `rep stosb` / `mov rax, r8`.
===
`move_bytes` forward case: `mov rax, rdi` (return value), `mov rcx, rdx`, `rep movsb`. `rdi` is the destination and `rsi` the source, as the instruction wants.
===
Copying forward is safe when `dst <= src`, or when `dst >= src + n` (no overlap). Otherwise copy backward: `std`, point both cursors at the last byte (`lea rdi, [rdi + rcx - 1]`, `lea rsi, [rsi + rcx - 1]`), `rep movsb`, then `cld`.
===
`str_length`: `xor eax, eax` / `mov rcx, -1` / `repne scasb` / `not rcx` / `dec rcx` / `mov rax, rcx`. For the empty string `scasb` examines only the NUL, so `rcx` is -2, `not` gives 1, and `dec` gives 0.
