Remember the last expression in a function is its return value, so leave off the semicolon. For the first function, use `2.0`-style float literals, not integers like `9/5`.
===
`c * 9.0 / 5.0 + 32.0` works. For the leap year, think of it as a single boolean expression combining `%` checks with `&&` and `||`.
===
A year is a leap year when `year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)`. Make that the function's final expression with no semicolon.
