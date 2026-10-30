Complete the starter:

- `unsafe fn for_each(xs: *const i32, len: usize, cb: Visit, ctx: *mut c_void) -> isize`: call `cb(ctx, x)` for each element in order. If `cb` returns a non-zero value, stop at once (that call still counts). Return the number of calls made. If `xs` is null and `len` is 0, make no calls and return 0. If `xs` is null and `len` is greater than 0, make no calls and return -1.
- `collect_stats`: a callback whose `ctx` is a `*mut Stats`; it updates `count`, `sum` and `max` and returns 0.
- `stats_of(xs: &[i32]) -> Stats`: use `for_each` and `collect_stats`. An empty slice gives `count 0, sum 0, max None`.
- `search_negative`: a callback whose `ctx` is a `*mut Search`; for a negative value it records `found = Some(seen)` and returns 1, otherwise it increments `seen` and returns 0.
- `first_negative(xs: &[i32]) -> Option<usize>`: use `for_each` and `search_negative`.

Sums use `i64` so they never overflow. The tests also try callbacks of their own.
