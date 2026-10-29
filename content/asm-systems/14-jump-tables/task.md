Implement two functions with **jump tables**:

- `long apply_op(long op, long a, long b)`: `op` selects the operation: 0 `a + b`, 1 `a - b`, 2 `a * b`, 3 `a & b`, 4 `a | b`, 5 `a ^ b`, 6 `a << (b & 63)`, 7 the signed maximum of `a` and `b`. Any other `op` (including negative values) returns `-1`.
- `const char *op_name(long op)`: returns the C string `"add"`, `"sub"`, `"mul"`, `"and"`, `"or"`, `"xor"`, `"shl"`, `"max"` for ops 0-7, and `"?"` for anything else.

Arguments: `op` in `rdi`, `a` in `rsi`, `b` in `rdx`. Dispatch with an indirect `jmp` through a table in `.rodata`, not a chain of compares. Preserve `rbx`, `rbp` and `r12`-`r15`.
