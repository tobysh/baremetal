Match `(op, value)` for unary rules and the two value kinds for binary rules. Check divisor zero before using `checked_div` or `checked_rem`.
===
`eval` needs only one recursive branch per AST shape. Use `?` separately for the left and right children before calling the shared arithmetic helper.
