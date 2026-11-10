A raw pointer plus a length is not a slice yet. Before anything else, decide what each function does for null, because every one of them must return quietly.
===
After the null check, `std::slice::from_raw_parts` (or `from_raw_parts_mut`) turns the pair into a safe slice, and then ordinary slice methods (`iter`, `fill`, `reverse`) do the work. Widen to `i64` before summing so large values cannot overflow.
===
`copy_raw` is the subtle one. Look at the `std::ptr` module for the two copy functions and read which one allows overlapping ranges; the tests use both overlap directions.
===
If only the overlap test fails, ask which element a forward loop reads after it has already overwritten it. The standard library already solved this; do not hand-roll the loop.
