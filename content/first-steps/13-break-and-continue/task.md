Read an integer `n`. Go through the numbers `1` to `n` and keep a running total:

- Skip every multiple of 3 (use `continue`).
- Stop completely (use `break`) as soon as adding the next number would push the total **above 30**. That number is not added or printed.
- Print each number you keep on its own line.

Finally print `total: ` and the total. For input `10` that is:

```
1
2
4
5
7
8
total: 27
```

Other values of `n` are tried, including `0`.
