**Track challenge: the left outer hash join.**

Keep `NewHashJoin` and add:

```go
func NewHashLeftOuterJoin(left, right Iterator, lcol, rcol, rightWidth int) Iterator
```

Every left row appears in the output. A left row with at least one match produces the same rows as the inner join. A left row with **no** match, including a left row whose key is NULL, produces one row: the left cells followed by `rightWidth` NULLs. Right rows with NULL keys are never matched. Order: left order, then right input order. Read each side exactly once, building from `right` on the first `Next`.

**Why this is harder:** the inner join can simply skip rows that cannot match; the outer join must remember that it emitted nothing for a left row and then emit exactly one padded row. A NULL key is a non-match, not a deletion.

**What the tests establish:** results against a brute-force reference with NULLs and duplicates, one pass per side, and a right side of unknown width being padded using `rightWidth` even when the right input is empty. The inner join tests are not repeated here.
