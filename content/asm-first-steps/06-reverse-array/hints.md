Handle `n = 0` first with `test rsi, rsi` and `jz`, so you never form the address `a - 8`.
===
Make an end pointer: `lea rsi, [rdi + rsi*8 - 8]`. Now `rdi` points at the first element and `rsi` at the last.
===
Loop: `cmp rdi, rsi` / `jae .Ldone`. Load both elements into two scratch registers (`rax`, `rcx`), store them crossed over, then `add rdi, 8` and `sub rsi, 8`.
