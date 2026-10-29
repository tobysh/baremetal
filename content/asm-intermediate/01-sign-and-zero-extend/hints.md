Keep the total in `rax` and the index in `rcx`, both cleared with `xor eax, eax` / `xor ecx, ecx`. Loop while `rcx < rsi`.
===
`sum_i8` loads with `movsx rdx, byte ptr [rdi + rcx]`. `sum_u8` is identical except for `movzx`. In `sum_u8` writing `movzx edx, byte ptr [...]` is enough, since writing `edx` clears the top half.
===
`sum_i32` loads with `movsxd rdx, dword ptr [rdi + rcx*4]` and then `add rax, rdx`. Adding a 32-bit register to `rax` directly would lose the sign.
