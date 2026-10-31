Specification of the pool (names are strings; a name exists after its first successful credit):

- Every balance is between 0 and `Cap` = 100.
- `Add(name, n)`: `n` must be greater than 0 and the new balance at most `Cap`, else an error and no change. History gets `"add n"`.
- `Use(name, n)`: `n` must be 0 or more. `n == 0` is a successful no-op (no history). Otherwise the balance must be at least `n` (using exactly the whole balance is allowed), else an error and no change. History gets `"use n"`.
- `Refund(name, n)`: `n` greater than 0; the balance becomes `min(balance+n, Cap)` (the excess is dropped). History gets `"refund n"` with the requested `n`.
- `Transfer(from, to, n)`: `n` greater than 0, `from` has at least `n`, and `to` would stay within `Cap`. If any check fails, **neither** balance changes. On success both histories get an entry: `"out n"` for `from` and `"in n"` for `to`.
- `History(name)` returns a copy: editing the result never changes the pool.
- `Report()` lists existing names in ascending order as `name=balance`, joined by commas.

Implement:

- `Found() []string`: the region ids with a real defect (any order).
- `Counterexample(id string) []Op`: at most 12 operations that expose that defect and only that defect (the op kinds are `add`, `use`, `refund`, `transfer`, `scribble`; `scribble` edits the first entry of the result of `History(Name)`).
- Fix the defects in `Pool`. Leave everything the specification already allows exactly as it is.

`main` runs a small sequence so you can try your counterexamples.
