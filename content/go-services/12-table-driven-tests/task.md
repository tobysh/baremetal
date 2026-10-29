The function under test is `Clamp(x, lo, hi int) int` in the starter. It limits `x` to the range from `lo` to `hi`, and if `lo > hi` it swaps them first.

- `ClampCases() []ClampCase` returns your table. Give every case a **unique, non-empty `Name`** and the correct `Want`. Cover the behaviour well: the hidden tests run your table against the real `Clamp` (everything must pass) and against several broken variants, and every broken variant must be caught by at least one of your cases.
- `CheckClamp(f func(x, lo, hi int) int, cases []ClampCase) []string` is the loop that runs a table. For each case whose result is wrong it adds the string `<Name>: Clamp(<X>, <Lo>, <Hi>) = <got>; want <Want>` to the result, in table order. When every case passes, the result is empty.

`ClampCase` is defined for you. You cannot see the broken variants, so think about where such a function can go wrong.
