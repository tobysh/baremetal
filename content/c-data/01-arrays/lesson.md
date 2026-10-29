---
title: Arrays and Indexing
summary: Store many values of one type side by side, reach each one by its index, and loop over them. Pass an array to a function together with its length.
minutes: 15
difficulty: easy
tags: arrays, loops
id: c-data/arrays
mode: harness
---
A variable holds one value. To hold a hundred test scores you would not want a hundred variables. An **array** is a row of boxes of the same type, laid out one after another in memory, under one name.

```c
int scores[5] = {70, 85, 90, 60, 100};

printf("%d\n", scores[0]);   // 70, the first box
printf("%d\n", scores[4]);   // 100, the last box
scores[1] = 88;              // change the second box
```

The number in the brackets is the **index**. Counting starts at **0**, so an array of 5 has indexes 0 to 4. Index 5 does not exist. C will not stop you from reading `scores[5]`; it just reads whatever memory happens to be next, which is a classic bug (and undefined behaviour). The sandbox runs your code with extra checks, so you may see an error if you step out of bounds.

## Looping over an array

The [loops lesson](/learn/first-steps/loops) gave you `for`. Together they are the everyday way to visit every element:

```c
int total = 0;
for (int i = 0; i < 5; i++) {
    total += scores[i];
}
```

## Arrays and functions

When a function takes an array, write empty brackets, and pass the length as a second parameter:

```c
int first_or_zero(int a[], int n) {
    return n > 0 ? a[0] : 0;
}
```

The function cannot work out how long the array is by itself, so you always hand the length over. Also, unlike a plain `int`, the function works on the **original array**, not a copy. (The next course explains why, using pointers.) For now: reading and writing `a[i]` inside the function affects the caller's array.

## Your turn

Write three functions. Each receives an array `a` and its length `n`.
