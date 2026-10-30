Read `n` with `scanf`, keep `int total = 0;` and loop with `for (int i = 1; i <= n; i++)`.
===
First line of the body: `if (i % 3 == 0) continue;`
===
Next, check `if (total + i > 30) break;` before you add `i` to `total`.
===
Only after both checks, add `i` to the total and print it. Print the `total:` line after the loop, so it also appears when `n` is 0.
