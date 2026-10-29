Complete `apply_unary`, `apply_binary` and `eval`.

Use the typed rules in the lesson and the supplied `RuntimeError` variants. Integer overflow is `Overflow`; zero division/remainder is `DivByZero`; wrong operand kinds are `TypeMismatch(String)`. The exact type-error text is up to you at this stage.

Evaluate binary children left to right. Identifiers return `Undefined(name.clone())`; a call returns `NotCallable`. Return `Value::Int` or `Value::Bool` as appropriate. Tests exercise negative division/remainder, all comparisons, wrong kinds, error order and `i64` boundaries.
