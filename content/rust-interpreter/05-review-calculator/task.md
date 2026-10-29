Implement `calc`, `calc_lines` and `calc_total`.

`calc` parses and evaluates a single expression. `calc_lines` skips whitespace-only lines and returns one string for every other line:

- Success: `N: VALUE` using `Value`'s display.
- Parse error: `N: error: parse error at POS: MESSAGE`.
- Runtime error: `N: error: TEXT` using the runtime error's display.

`N` is the original one-based line number; `POS` is the byte position within the line. A bad line must not suppress later lines.

`calc_total` sums successful integer results, ignoring booleans and failed lines. Return `None` for no integers or sum overflow. Tests vary inputs and include zero, negative totals and overflow.
