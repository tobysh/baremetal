`fill_squares` is a plain loop with `a[i] = i * i;`. For `reverse_array`, use `left = 0` and `right = n - 1`, swap through a temporary variable, then move both inwards while `left < right`.
===
`rotate_left`: save `a[0]` in a variable first, copy each element down one place going upwards (`a[i] = a[i + 1]` for `i` up to `n - 2`), then put the saved value into `a[n - 1]`. For `remove_value`, keep a second counter for where the next kept value goes.
===
```c
int remove_value(int a[], int n, int v) {
    int keep = 0;
    for (int i = 0; i < n; i++) {
        if (a[i] != v) {
            a[keep] = a[i];
            keep++;
        }
    }
    return keep;
}
```
