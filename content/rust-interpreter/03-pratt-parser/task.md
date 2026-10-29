Implement `parse_expr(s, min_bp)` using the supplied powers and `infix` table.

Support `Num`, `true`, `false`, identifiers, prefix `-` and `!`, parentheses, binary operators, and chained calls with zero or more comma-separated arguments. Trailing commas are rejected. Use `expect` for closing parentheses.

For an invalid expression start return `ParseError::new(format!("unexpected {:?}", token), offset)`. Missing input reports `unexpected end of input` at `s.pos()`.

Stop before a binary operator whose left binding power is below `min_bp`; handle calls only when `CALL_BP >= min_bp`. Tests vary precedence, associativity, nested calls and malformed inputs.
