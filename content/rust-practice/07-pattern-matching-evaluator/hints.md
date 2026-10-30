Start with `eval`. Match on `e`: `Expr::Num(n) => Ok(*n)`. For `Add(a, b)`, `let x = eval(a)?; let y = eval(b)?; x.checked_add(y).ok_or(EvalError::Overflow)`. The other binary operations follow the same pattern, and `a` and `b` are `&Box<Expr>`, which `eval` accepts thanks to auto-deref.
===
`Div`: after evaluating both sides, `if y == 0 { return Err(EvalError::DivByZero); }` and then `x.checked_div(y).ok_or(EvalError::Overflow)`. For `Neg`, use `checked_neg`.
===
`simplify` takes `Expr` by value. For `Expr::Add(a, b)`, write `let (a, b) = (simplify(*a), simplify(*b));` then `match (a, b) { (Expr::Num(0), x) | (x, Expr::Num(0)) => x, (a, b) => add(a, b) }`. The `add` helper rebuilds the node with fresh boxes.
===
`Mul` has two rules: `(Num(0), _) | (_, Num(0)) => Num(0)` goes first, then `(Num(1), x) | (x, Num(1)) => x`. For `Neg`, `match simplify(*inner) { Expr::Neg(x) => *x, Expr::Num(n) => Expr::Num(n.wrapping_neg()), other => neg(other) }`.
===
`describe`: `match items { [] => ..., [x] => format!("one: {}", x), [a, b] => ..., [first, .., last] if first == last => ..., [first, rest @ ..] => format!("{} then {} more", first, rest.len()) }`. Order matters: the bookends arm must come before the catch-all.
