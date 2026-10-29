Complete `parse_fn`, `parse_return`, `exec_fn`, `exec_return`, `call_function` and `eval_call`.

Functions have a required name, parenthesized comma-separated parameter names and a braced body. A trailing parameter comma is invalid. `return` optionally contains an expression before its required semicolon.

Capture the defining environment by shared handle. Calling creates a child of that captured environment; bind parameters to already evaluated arguments. Wrong counts return `Arity { expected, got }`. A non-function callee returns `NotCallable`. Evaluate the callee first and every argument left to right before callable/arity checks. Bare return and reaching the body's end produce `Nil`.

Tests cover recursion, mutual recursion, returned closures, independent counters, shared captured mutation and errors inside calls.
