`prefix_sums`: keep `int running = 0;`, and in each step add `a[i]` to it and store it in `out[i]`. `arrays_equal` returns 0 at the first index where the arrays differ, and 1 after the loop.
===
`count_peaks`: loop `i` from 1 while `i + 1 < n`, so both neighbours exist. For `second_largest`, a simple plan is two passes: first find the maximum, then find the largest value that is *less than* that maximum. Remember values can be negative, so don't start the second best at 0 without a flag.
===
```c
int count_peaks(int a[], int n) {
    int count = 0;
    for (int i = 1; i + 1 < n; i++) {
        if (a[i] > a[i - 1] && a[i] > a[i + 1]) count++;
    }
    return count;
}
```
