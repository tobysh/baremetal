`times10`: `lea rax, [rdi + rdi*4]` gives `x*5`; then `add rax, rax` doubles it.
===
`nine_plus_five` is exactly one instruction: `lea rax, [rdi + rdi*8 + 5]`.
===
`times85`: let `t = x*5` (`lea rax, [rdi + rdi*4]`). Then `85*x = 17*t`, and `17*t = t*16 + t`: `mov rdx, rax` / `shl rax, 4` / `add rax, rdx`.
===
`elem_addr`: `lea rax, [rdi + rsi*8]`. A negative index in `rsi` works unchanged, because the addition is signed.
