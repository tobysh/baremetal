Implement `Choose(s Stats) Plan`. It returns the cheapest join strategy, the access path for the left table, and the cost of the join.

Inputs: `L`, `R`, `Matches` and `Selected` are non-negative (counts at most one million; `Matches` is the expected matching right rows per left row, `Selected` the left rows an index range would return). Tables can be empty (`L` or `R` equal to 0).

Candidate join costs, all in `int64`:

- `nested`: `L*R`
- `hash`: `3*(L+R)`
- `merge`: `L+R`, plus `4*L` when `SortedLeft` is false, plus `4*R` when `SortedRight` is false
- `index`: `L*(2+Matches)`, a candidate **only** when `IndexRight` is true

`Plan.Join` is the name of the minimum-cost candidate and `Plan.Cost` its cost. Ties go to the earlier candidate in the order nested, hash, merge, index.

`Plan.Access` is `"index"` when `IndexLeft` is true and `2+Selected < L`; otherwise `"scan"`. This does not depend on the join choice.

The tests try thousands of random statistics plus hand-picked ties, empty tables and million-row cases.
