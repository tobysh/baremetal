Complete these in the starter. Every one returns an iterator and must not collect:

- `fn multiples_of(k: u32) -> impl Iterator<Item = u32>`: `k, 2k, 3k, ...` forever, starting with `k`. (For `k = 0` it yields `0` forever; that is fine.)
- `fn collatz(start: u64) -> impl Iterator<Item = u64>`: the Collatz sequence from `start` down to and including `1` (even: halve; odd: `3n + 1`). For `start = 0` yield nothing. Hint: `std::iter::successors`.
- `fn digits_rev(n: u32) -> impl Iterator<Item = u32>`: the decimal digits of `n`, least significant first. `0` gives a single `0`. Hint: `successors` again, or a `from_fn` counter.
- `fn pick(kind: &str, limit: u32) -> Box<dyn Iterator<Item = u32>>`: for `"even"` the even numbers below `limit`, for `"odd"` the odd numbers below `limit`, for `"all"` every number below `limit`; any other `kind` yields an empty iterator.
