The program is `p->code[t][s->pc[t]]`. Name it `i`, and use `int *r = s->reg[t];` for the registers.
===
`step` is a `switch (i->op)`. Compute `next = s->pc[t] + 1` first; only the jump cases replace it.
===
`SLTI` stores 1 or 0 in `r[a]`. `JZ`/`JNZ` look at `r[a]` and jump to `b`. `JMP` jumps to `a`. `STOREX` writes to `m[a + r[b]]`.
===
`CAS`: `if (s->mem[i->a] == r[i->b]) { s->mem[i->a] = r[i->c]; r[i->d] = 1; } else r[i->d] = 0;`
