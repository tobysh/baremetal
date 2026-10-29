All of these change the array you pass in. None may use another array.

- `void reverse_array(int a[], int n)` reverses the first `n` elements in place.
- `void rotate_left(int a[], int n)` moves every element one place to the left, and the old first element goes to the end: `1 2 3 4` becomes `2 3 4 1`. Lengths 0 and 1 change nothing.
- `void fill_squares(int a[], int n)` sets `a[i]` to `i * i` for every index `0` to `n - 1`.
- `int remove_value(int a[], int n, int v)` removes every element equal to `v`, keeps the others in their original order at the front of the array, and returns the new length. What lies after the new length does not matter.
