Read `n` first, then loop `i` from 0 to `n - 1` with `scanf("%15s %d", students[i].name, &students[i].score)`. (Arrays like `name` are passed to `scanf` without an `&`; the number needs one.)
===
Sort with insertion sort on `students[j].score < current.score`, moving a record only past strictly smaller scores: that keeps ties in input order. Then one loop prints `i + 1`, the name and the score, while adding up the total and the pass count.
===
For the average, convert before dividing: `(double)total / n`, and guard `n == 0`.

```c
double average = n > 0 ? (double)total / n : 0.0;
printf("average: %.2f\n", average);
printf("passed: %d of %d\n", passed, n);
```
