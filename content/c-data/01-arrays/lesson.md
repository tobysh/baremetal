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

## What's in memory

An array is one block of memory split into equal boxes with no gaps. `scores` has five `int` boxes of 4 bytes each, so 20 bytes in a row. The addresses below are made up, only the pattern matters:

```text
address   box        holds
0x2000    scores[0]    70
0x2004    scores[1]    85
0x2008    scores[2]    90
0x200C    scores[3]    60
0x2010    scores[4]   100
0x2014    (not part of scores)
```

`scores[i]` means "start at the first box and move over `i` boxes". Nothing checks that `i` is in range. `scores[5]` lands on `0x2014`, which belongs to something else.

That is **undefined behaviour**, and it means the language promises nothing at all. The read might return a leftover number, crash, or seem to work today and fail tomorrow, so "it seemed fine" is never evidence. Here the sandbox builds your code with a checker, so you get a message such as `runtime error: index 5 out of bounds for type 'int [5]'`. On a normal build you would get no message.

When you pass an array to a function, the function does not receive a copy of the 20 bytes. It receives the place where the first box lives, plus the length you pass:

```text
main's frame                       total's frame
scores  [70 85 90 60 100]          a  -> the first box of scores
                                   n  5
```

That is why a function that writes `a[i]` changes the caller's array.

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
