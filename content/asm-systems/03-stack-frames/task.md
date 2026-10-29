Implement `size_t to_decimal(char *buf, unsigned long v)`. Write the decimal digits of `v` into `buf`, followed by a terminating NUL byte, and return the number of digits (not counting the NUL). `0` is written as `"0"`. The caller's buffer is large enough for 21 bytes and nothing past the terminator may be written.

`buf` is in `rdi`, `v` in `rsi`; return the length in `rax`. Build a real stack frame (`push rbp` / `mov rbp, rsp` / `sub rsp, ...` / `leave`) and use a scratch buffer on the stack. Preserve `rbx`, `rbp` and `r12`-`r15`.
