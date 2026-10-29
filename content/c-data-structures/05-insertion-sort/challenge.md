**Challenge: count inversions fast.**

The lesson's `insertion_sort` counts inversions (pairs `i < j` with `a[i] > a[j]`) by doing the sorting work, which takes about `n * n / 2` steps. That is fine for a few hundred numbers and hopeless for a few hundred thousand. Write

```c
long count_inversions(const int *a, int n);
```

It must run in about `n log n` time. The idea: split the array in half, count the inversions inside each half, then count the pairs that cross the middle while **merging** the two sorted halves. Whenever you take an element from the right half before the remaining left elements, every one of those left elements forms an inversion with it.

- `a` is `const`: leave the caller's array exactly as it was, and sort a copy. You may `malloc` it (and must `free` it).
- Equal values are not an inversion.
- The answer can exceed 32 bits, hence `long`. `n` may be 0 or 1.
- The tests include an array of 200000 elements, so a quadratic solution runs out of time.
