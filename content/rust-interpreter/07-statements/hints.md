Dispatch on `s.peek()`; assignment needs both `Ident(_)` and `peek_at(1) == Some(&Token::Eq)`. Then consume the name with `expect_ident`.
===
Evaluate before changing the environment. `let` calls `define` even if the name already exists locally; `assign` must report an error if its boolean result is false.
