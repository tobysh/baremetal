Implement these with `lea`, shifts and adds (an `imul` also passes the tests, but try the `lea` forms):

- `long times10(long x)` returns `x * 10`.
- `long nine_plus_five(long x)` returns `x * 9 + 5` in a single `lea`.
- `long times85(long x)` returns `x * 85`.
- `long *elem_addr(long *base, long i)` returns the address of `base[i]` (negative `i` allowed).

Results wrap around at 64 bits like C does.
