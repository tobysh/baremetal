`bit_count` is two instructions: `popcnt rax, rdi` and `ret`.
===
`lowest_set_index`: `bsf rax, rdi` then `jnz .Ldone`; otherwise `mov eax, -1`. `.Ldone: ret`. The flag is set by `bsf` itself, so no `test` is needed.
===
`next_pow2`: `cmp rdi, 1` / `ja .Lbig`; else `mov eax, 1` / `ret`. At `.Lbig`: `lea rax, [rdi - 1]`, `bsr rcx, rax`, `inc ecx`, `mov eax, 1`, `shl rax, cl`, `ret`.
