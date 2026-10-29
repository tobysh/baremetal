`clamp_byte`: `xor ecx, ecx` and `mov edx, 255` first, then `mov rax, rdi` / `test rax, rax` / `cmovs rax, rcx`. Finally `cmp rax, rdx` / `cmovg rax, rdx`.
===
`count_in_range`: use `r9` as the index and `rax` as the count. In the loop: `movsx r8d, byte ptr [rdi + r9]`, `cmp r8d, edx` / `setge r10b`, `cmp r8d, ecx` / `setle r11b`, `and r10b, r11b`, `movzx r10d, r10b`, `add rax, r10`.
===
`nibble_at`: `lea ecx, [rsi*4]`, `shr rdi, cl`, `mov eax, edi`, `and eax, 15`.
