`CheckClamp`: `var fails []string`, loop over `cases`, `got := f(c.X, c.Lo, c.Hi)`, and `if got != c.Want { fails = append(fails, fmt.Sprintf("%s: Clamp(%d, %d, %d) = %d; want %d", c.Name, c.X, c.Lo, c.Hi, got, c.Want)) }`.
===
Cases for `ClampCases` worth having: a value inside the range; below it; above it; exactly equal to `lo`; exactly equal to `hi`; `lo == hi`; negative numbers (a negative value inside a negative range, and a range that crosses zero).
===
Do not forget the swapped range: `{"swapped", 3, 5, 1, 3}` should leave 3 alone, and `{"swapped below", 0, 5, 1, 1}` should give 1. Work out each `Want` by hand from the description, not by running `Clamp`.
