Copy `x` into `rax`, then use `neg rax`. That gives you `-x` and sets the flags from it.
===
After `neg rax`, the sign flag `SF` is set when `-x` is negative. That means `x` was positive and you want `x` back.
===
`cmovs rax, rdi` copies `rdi` into `rax` only when the sign flag is set. Four instructions in total: `mov`, `neg`, `cmovs`, `ret`.
