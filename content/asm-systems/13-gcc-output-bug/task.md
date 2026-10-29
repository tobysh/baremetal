The function `count_above(const int *a, int n, int limit)` in the editor is gcc `-O0` style assembly with **three bugs**. It should return how many of the `n` signed `int`s in `a` are strictly greater than `limit`. Find and fix the bugs; keep the structure of the function (frame pointer, stack slots) if you like, or simplify it, as long as it behaves correctly.

`a` is in `rdi`, `n` in `esi`, `limit` in `edx`; the result is returned in `eax`. `n` may be 0 or negative (then the answer is 0). Preserve `rbx`, `rbp` and `r12`-`r15`.
