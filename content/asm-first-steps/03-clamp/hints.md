Start with `mov rax, rdi`, so that `rax` already holds the answer in the common case where `x` is in range.
===
Check the lower bound first: `cmp rax, rsi`, then `jge` over a `mov rax, rsi`. Give the label a `.L` name such as `.Lcheck_high`.
===
Then check the upper bound the same way: `cmp rax, rdx`, `jle` to the end, otherwise `mov rax, rdx`. Use signed jumps (`jl`, `jg`), not `jb`/`ja`.
