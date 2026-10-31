Implement both designs and the chooser. Keys are always positive; `n` never exceeds `MAXN` (2048).

- `sa_init/sa_insert/sa_has/sa_range` on `sorted_set`, `hb_init/hb_insert/hb_has/hb_range` on `hash_set`, with the same results as a plain reference.
- Count honestly: `probes`, `moves`, `init` as defined in the lesson. `sa_insert` of a duplicate moves nothing. `hb_init` sets `c.init` to `SLOTS`. `hb_range` adds exactly `SLOTS` probes.
- `sa_range(lo, hi)` counts keys in `[lo, hi]` using two binary searches (no linear scan); it returns 0 when `lo > hi`.
- `choose_design(const op_t *ops, int nops, long mem_limit)` runs the operations on a fresh `sorted_set` and a fresh `hash_set`, then returns `'A'`, `'B'` or `'?'` as described. Memory is `4 * sa.n` for A and `4 * SLOTS` for B after the run.
- Do not use `hash_key` with a different function; the tests assume the provided one.

Written justification is not graded by the tests; add your reasoning as comments above `choose_design`.
