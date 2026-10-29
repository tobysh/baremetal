---
title: Changing Arrays in Place
summary: Write into array elements, swap two of them, and shuffle values around inside the same array without needing a second one.
minutes: 20
difficulty: medium
tags: arrays, algorithms
id: c-data/changing-arrays
mode: harness
---
So far you have only read arrays. Because a function works on the caller's **original** array, it can also change it. `a[i] = value;` writes into a box, and the caller sees the new value afterwards.

## Swapping two elements

To swap two boxes you need a third place to hold one value while you overwrite it:

```c
int tmp = a[i];
a[i] = a[j];
a[j] = tmp;
```

Without `tmp` you would overwrite `a[i]` and lose it.

## Reversing: two indexes walking inwards

To reverse an array, swap the first with the last, then the second with the second to last, and so on. Use two indexes that meet in the middle:

```c
int left = 0, right = n - 1;
while (left < right) {
    // swap a[left] and a[right]
    left++;
    right--;
}
```

You have seen this shape with text; here it works on numbers. Stop when `left >= right`, otherwise you would swap everything back.

## Shifting and compacting

Moving every element one place needs care about **order**. To shift left, copy `a[1]` into `a[0]`, then `a[2]` into `a[1]`, going upwards. Going the other way would copy the same value everywhere. To delete matching values, keep a second index `keep` for where the next surviving value goes, and copy forward while reading ahead:

```c
int keep = 0;
for (int i = 0; i < n; i++) {
    if (a[i] != v) {
        a[keep] = a[i];
        keep++;
    }
}
```

After the loop, `keep` is the new length.

Searching and loops are from [the previous lesson](/learn/c-data/searching).
