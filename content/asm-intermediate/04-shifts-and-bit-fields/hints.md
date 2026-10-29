`get_bits`: save nothing, but note the registers: `x` in `rdi`, `lo` in `rsi`, `len` in `rdx`. `mov ecx, esi` / `shr rdi, cl` shifts the field to the bottom.
===
Build the mask in `rax`: `mov ecx, edx` / `mov eax, 1` / `shl rax, cl` / `dec rax`. With `len = 0` the mask is `1 - 1 = 0`, which is right. Finish with `and rax, rdi`.
===
`rotl32`: `mov ecx, esi` / `mov eax, edi` / `rol eax, cl`. The CPU already reduces the count modulo 32 for 32-bit operands.
===
`set_bits` has four arguments, so `v` is in `rcx`, the very register you need for shifting. Move it first: `mov r8, rcx`. Then build the mask in `rax`, `and r8, rax` to trim `v`, move `lo` into `cl`, `shl rax, cl` and `shl r8, cl`, then `not rax` / `and rdi, rax` / `or rdi, r8`.
