You'll need four `printf` calls, one per line, each ending in `\n`.
===
Each line has three numbers, so each `printf` needs three `%d` placeholders: `printf("%d + %d = %d\n", a, b, a + b);`
===
Integer division drops the fraction, so `a / b` with 7 and 5 really is 1. That's expected!
