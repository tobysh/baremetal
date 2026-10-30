The starter has every method with a value receiver and empty bodies. Fix the receivers and fill in the code.

- `Counter` has an `n` field. `Inc()` must add 1 to the caller's counter, and `Value()` returns `n`.
- `IncAll(cs []Counter)` increments every counter in the slice, in the slice itself (not copies).
- `Rect` has `W` and `H`. `Area()` returns `W * H` and stays a value-receiver method, so both `Rect` and `*Rect` have it. `Scale(f float64)` multiplies both sides by `f` in place, so `*Rect` satisfies `Scaler` and plain `Rect` does not.
- `Grow(items []Scaler, f float64)` scales every item by `f`.
- `Stack` holds `items []int`. `Push(v int)`, `Pop() (int, bool)` (the last pushed value, or `0, false` when empty) and `Len() int` all use pointer receivers, so a zero `Stack` is ready to use.
- `Fill(p Pusher, n int)` pushes `1, 2, ..., n` onto `p` (nothing if `n <= 0`), where `Pusher` is the interface `Push(int)`.
