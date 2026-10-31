Implement `NewHashJoin(left, right Iterator, lcol, rcol int) Iterator`, an inner equi-join of `left[lcol] = right[rcol]`.

- Output `left ++ right` as a new slice. Order: left order, then right input order within a key.
- On the first `Next`, read all of `right` into a hash table; `Open` reads nothing. Then stream `left`.
- Each input is opened once and each row read once.
- NULL keys on either side never match. Keys of different types never match.
- `Open` after `Close` starts over; `Close` closes both inputs.

`EqCols` and `concatRows` are provided.
