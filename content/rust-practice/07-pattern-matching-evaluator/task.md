The types and the little constructor helpers (`num`, `neg`, `add`, `sub`, `mul`, `div`) are given in the starter. `Expr` and `EvalError` derive `Debug`, `PartialEq` and `Clone`.

- `fn eval(e: &Expr) -> Result<i64, EvalError>` evaluates the tree. Use checked arithmetic: any overflow (including negating `i64::MIN`, or `i64::MIN / -1`) is `Err(EvalError::Overflow)`. Division by zero is `Err(EvalError::DivByZero)`. The first error found wins, evaluating the left side before the right.
- `fn simplify(e: Expr) -> Expr` rewrites the tree bottom-up (simplify the children first, then look at the result). The rules:
  - `x + 0` and `0 + x` become `x`
  - `x - 0` becomes `x`
  - `x * 1` and `1 * x` become `x`
  - `x * 0` and `0 * x` become `Num(0)`
  - `x / 1` becomes `x`
  - `Neg(Neg(x))` becomes `x`, and `Neg(Num(n))` becomes `Num(-n)` (use `wrapping_neg`)
  - anything else keeps its shape with simplified children
  Simplification never evaluates other operations: `1 + 2` stays `Add(Num(1), Num(2))`.
- `fn describe(items: &[i32]) -> String` using slice patterns, tried in this order:
  - empty gives `"empty"`
  - one item gives `"one: X"`
  - two items give `"two: X,Y"`
  - three or more where the first and last are equal give `"bookends: X"`
  - otherwise `"X then N more"`, where `N` is how many items follow the first
  
  So `[7]` is `"one: 7"`, `[1, 2, 3, 1]` is `"bookends: 1"`, and `[4, 5, 6]` is `"4 then 2 more"`.
