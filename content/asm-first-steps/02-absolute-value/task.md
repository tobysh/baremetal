Implement `long abs_val(long x)` in assembly. It returns the absolute value of `x`: `x` when `x >= 0`, and `-x` otherwise.

`x` arrives in `rdi`; leave the result in `rax`. Try it without a jump: `neg` followed by a conditional move is enough. You can ignore the one value that has no positive twin (`LONG_MIN`).
