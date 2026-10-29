`u64_to_dec`: keep `rax` = value, `r9` = write cursor, `r8` = start of the buffer, `r10` = 10. In the loop: `xor edx, edx` / `div r10` / `add dl, 48` / `mov [r9], dl` / `inc r9` / loop while `rax` is non-zero.
===
After the loop store the NUL: `mov byte ptr [r9], 0`. The length is `r9 - r8`; compute it into `rax` right now (`mov rax, r9` / `sub rax, r8`), because you will move `r8` while reversing.
===
Reverse: `lea rcx, [r9 - 1]` is the last digit. While `r8 < rcx` swap `[r8]` and `[rcx]` (load both bytes into `dl` and `r11b`, store crosswise), `inc r8`, `dec rcx`.
===
`parse_dec`: `xor eax, eax` (value) and `xor r8d, r8d` (negative flag). Look at the first character with `movzx ecx, byte ptr [rdi]`: for `-` (45) set the flag and advance, for `+` (43) just advance.
===
Digit loop: `movzx ecx, byte ptr [rdi]` / `sub ecx, 48` / `cmp ecx, 9` / `ja .Lend`, then the two `lea`s from the lesson, `inc rdi`, loop. At `.Lend`, `neg rax` if the flag is set.
