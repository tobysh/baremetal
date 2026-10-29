Implement `parse_stmt` and `exec_stmt` for the supplied `Stmt` variants.

Parse `let name = expr;`, `name = expr;`, `print expr;` and `expr;`. Use one-token lookahead beyond the identifier to recognize assignment. Require every semicolon and propagate the supplied parser errors.

Execute each statement against `env`, returning `Flow::Normal` on success. Assignment evaluates its right-hand side before checking that the target exists. Missing names are `RuntimeError::Undefined`; other evaluation errors propagate unchanged. Prints append display text to the shared output buffer.

Tests vary statement sequences, redeclarations, assignments, missing delimiters and error order.
