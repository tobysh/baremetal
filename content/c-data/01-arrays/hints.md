Start `array_sum` with `int total = 0;`, then loop `i` from 0 while `i < n` and add `a[i]`. Return `total` after the loop.
===
For `array_max`, don't start the best value at 0: the elements may all be negative. Start with `a[0]`, then loop from index 1 and replace the best whenever `a[i]` is bigger. `count_above` is the sum loop again, but adding 1 only when `a[i] > limit`.
===
```c
int array_max(int a[], int n) {
    int best = a[0];
    for (int i = 1; i < n; i++) {
        if (a[i] > best) best = a[i];
    }
    return best;
}
```
