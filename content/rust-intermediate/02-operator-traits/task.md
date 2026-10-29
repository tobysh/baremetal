Implement the traits for `Cents(i64)` (it already derives `Debug`, `Clone`, `Copy`, `PartialEq`, `Eq`, `PartialOrd` and `Ord`).

- `Add`, `Sub`: add and subtract amounts.
- `Mul<i64>`: multiply by a whole number, so `Cents(250) * 3 == Cents(750)`.
- `Neg`: `-Cents(5) == Cents(-5)`.
- `AddAssign`: `a += b`.
- `Sum`: `amounts.into_iter().sum::<Cents>()`; an empty iterator sums to `Cents(0)`.
- `Display`: dollars and two-digit cents with a `$`: `Cents(1234)` prints `$12.34`, `Cents(5)` prints `$0.05`, and a negative amount puts the sign first: `Cents(-5)` prints `-$0.05`.

Hidden tests use many different amounts, including zero and negatives.
