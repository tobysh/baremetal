The first argument is in `rdi` and the second in `rsi`. The answer must end up in `rax` before `ret`.
===
In Intel syntax the destination is on the left: `mov rax, rdi` copies `rdi` into `rax`.
===
Copy one argument into `rax`, then use `add rax, <the other argument>`. Two instructions plus `ret`.
