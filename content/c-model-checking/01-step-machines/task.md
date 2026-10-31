Implement the two functions in the starter.

- `thread_enabled(p, s, t)` is `1` if thread `t` can take a step now and `0` otherwise. A thread at `OP_END` is finished, so it is not enabled. A thread at `OP_LOCK a` is enabled only while `s->lock[a] == 0`. Every other instruction is always enabled.
- `step(p, s, t)` executes the instruction at `s->pc[t]` and updates `pc`. `LOCK` stores `t + 1` as the owner; `UNLOCK` stores 0; `CAS` compares `m[a]` with `r[b]`, stores `r[c]` on a match, and sets `r[d]` to 1 or 0; jumps change `pc` to their target instead of adding 1.

Hidden tests run many fixed pseudo-random schedules over programs with locks, branches, CAS loops and indexed stores, and compare every step against a separate reference interpreter.
