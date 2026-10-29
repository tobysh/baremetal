Translate these two C functions into assembly. `struct item` is `{ int weight; int bonus; long value; }` (16 bytes).

- `long total_score(const struct item *p, size_t n)`: the sum, over items with `weight > 0`, of `(long)weight * value + bonus`.
- `long heaviest(const struct item *p, size_t n)`: the index of the first item with the largest (signed) `weight`, or `-1` if `n` is 0.

`p` is in `rdi`, `n` in `rsi`, result in `rax`. Preserve `rbx`, `rbp` and `r12`-`r15`.
