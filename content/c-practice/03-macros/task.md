Fix these five macros in the starter. They must work for arguments that are whole expressions, such as `SQUARE(1 + 2)`.

- `SQUARE(x)` is `x` times `x`. `SQUARE(1 + 2)` must be `9`, and `100 / SQUARE(5)` must be `4`.
- `MAX(a, b)` is the larger of the two. `MAX(5, 2) + 10` must be `15`. Bracket the whole expression.
- `ARRAY_LEN(arr)` is the number of items in a real array: `sizeof(arr) / sizeof((arr)[0])`.
- `SWAP_INT(a, b)` swaps two `int` variables, even when one of them is named `t`. Keep it a single statement with `do { ... } while (0)`, and use a temporary name that callers will not choose.
- `STRINGIFY(x)` turns its argument into a string literal: `STRINGIFY(hello)` is `"hello"`.

You do not need to handle arguments with side effects (`SQUARE(i++)`).
