At the first instruction, `[rsp]` is the return address. The seventh argument `g` is at `[rsp + 8]` and the eighth `h` is at `[rsp + 16]`. Load them into scratch registers such as `r10`.
===
Start with `mov rax, rdi` (weight 1). Then for each other argument multiply and add: `lea rax, [rax + rsi*2]` handles `b`; for `c` use `lea rdx, [rdx + rdx*2]` then `add rax, rdx`; for the rest `imul r9, r9, 6` and `add rax, r9` work.
===
For the stack arguments: `mov r10, [rsp + 8]`, `imul r10, r10, 7`, `add rax, r10`, then `mov r10, [rsp + 16]`, `shl r10, 3` (times 8), `add rax, r10`, `ret`. Do not `push` anything, so the offsets stay valid.
