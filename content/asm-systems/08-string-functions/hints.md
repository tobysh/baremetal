`my_strlen`: `xor eax, eax`; loop: `cmp byte ptr [rdi + rax], 0`, `je` to `ret`, `inc rax`, `jmp` back.
===
`my_memcpy`: the arguments are already `rdi` (dst), `rsi` (src), `rdx` (n). `movsb` needs the count in `rcx`: `mov rcx, rdx`. Save the return value first with `mov rax, rdi`, since `rep movsb` advances `rdi`. Then `rep movsb` and `ret`.
===
`my_strcmp`: loop with `movzx eax, byte ptr [rdi]` and `movzx ecx, byte ptr [rsi]`. `cmp eax, ecx` / `jne .Ldiff`; `test eax, eax` / `jz .Lequal` (eax is 0 already, so just `ret`); advance both pointers with `inc` and repeat. `.Ldiff: sub eax, ecx` / `ret`.
