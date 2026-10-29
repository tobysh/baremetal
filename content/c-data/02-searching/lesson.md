---
title: Searching and Checking Arrays
summary: Find where a value lives, count how often it appears, and test a property of the whole array, using loops that stop early.
minutes: 15
difficulty: easy
tags: arrays, algorithms
id: c-data/searching
mode: harness
---
Most array work is one of two questions: **where is this?** and **is this true for everything?**

## Linear search

To find a value, look at each element in turn. When you find it, you can `return` straight away, so the loop stops early:

```c
int index_of(int a[], int n, int target) {
    for (int i = 0; i < n; i++) {
        if (a[i] == target) {
            return i;       // found it, no need to look further
        }
    }
    return -1;              // looked at everything: not there
}
```

Returning `-1` for "not found" is a common convention because `-1` can never be a valid index.

## Checking a property of the whole array

"Is every element positive?" is the opposite shape. One counterexample settles it, so return as soon as you see one. Only if the loop finishes can you answer yes:

```c
int all_positive(int a[], int n) {
    for (int i = 0; i < n; i++) {
        if (a[i] <= 0) return 0;
    }
    return 1;
}
```

An empty array passes: nothing breaks the rule. Look at two neighbours with `a[i]` and `a[i + 1]`, and stop the loop one step early so that `i + 1` is still a valid index.

Everything in [the arrays lesson](/learn/c-data/arrays) about indexes and passing the length still applies.
