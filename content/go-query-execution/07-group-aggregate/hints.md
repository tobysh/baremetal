A `Value` is a struct, so `Value{N: 3, Valid: false}` and `Value{}` are different map keys. Before looking a key up, turn every NULL key into `Value{}`.
===
Use two structures together: a `[]Group` that keeps first-seen order, and a `map[Value]int` from the (normalised) key to its position in that slice. On a miss, append a new `Group{Key: k}` and record `len(out)-1`.
===
Per row, always do `Rows++`. Only when `r.Value.Valid` do `Count++`, add to `Sum.N` and set `Sum.Valid = true`. A group of only NULL values then keeps the zero `Sum`, which is NULL.
===
If the tests complain about the input changing, check you never write through `rows[i]`; you only need to read from it.
