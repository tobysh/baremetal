`arena_init` stores the three fields with `off = 0`. `arena_used` returns `off`. `arena_reset` sets `off` to 0.
===
In `arena_alloc`, work with the real address: `addr = (uintptr_t)a->base + a->off`, `pad = (align - (addr & (align - 1))) & (align - 1)`. Reject `align` of 0 or not a power of two first.
===
Avoid overflow: let `room = a->cap - a->off` (always valid). Fail if `pad > room` or `size > room - pad`. Only after these checks, commit with `a->off += pad + size`.
===
`arena_rewind`: `if (mark <= a->off) a->off = mark;`
