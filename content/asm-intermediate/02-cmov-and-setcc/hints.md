`max3`: `mov rax, rdi`, `cmp rax, rsi`, `cmovl rax, rsi`, then compare with `rdx` and `cmovl` again.
===
`sign_of`: clear `eax` and `ecx` first, then `test rdi, rdi`, `setg al`, `setl cl`, `sub eax, ecx`. The result is an `int`, so `eax` is what counts.
===
`sat_sub`: put a zero in a spare register with `xor ecx, ecx` **before** the subtraction, then `mov rax, rdi` / `sub rax, rsi` / `cmovb rax, rcx`. `sub` sets the carry flag when it borrows, which is the "below" condition.
