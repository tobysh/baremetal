Use two loops. The outer one runs `r` from 1 to `rows`; the inner one runs `c` from 1 to `cols` and prints `r * c` with `printf("%4d", ...)`. Print the newline after the inner loop finishes, not inside it.
===
Don't hardcode 3 and 4: the check changes `rows` and `cols`. The loop conditions should read `r <= rows` and `c <= cols`.
===
```c
for (int r = 1; r <= rows; r++) {
    for (int c = 1; c <= cols; c++) {
        printf("%4d", r * c);
    }
    printf("\n");
}
```
