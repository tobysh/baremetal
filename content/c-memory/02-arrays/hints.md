Loop over the indices `0` up to (but not including) `len`, and look at `arr[i]` each time.
===
For the sum, keep a running total that starts at 0. For the maximum, do not start at 0 (all numbers might be negative). Start with the first element, then compare the rest against it.
===
```c
int best = arr[0];
for (size_t i = 1; i < len; i++) {
    if (arr[i] > best) {
        best = ...;
    }
}
return best;
```
