---
title: Copying and Comparing Strings
summary: Strings cannot be copied or compared with = and ==, so write the loops yourself: copy, compare and append, taking care with the terminator.
minutes: 20
difficulty: medium
tags: strings, arrays
id: c-data/copy-compare
mode: harness
---
With numbers you write `b = a;` and `if (a == b)`. With strings, neither does what you hope. A string variable is an array, and arrays are not copied by `=` or compared by `==` (that would compare *where* they live, not what they hold). You must go character by character, like you did in [the previous lesson](/learn/c-data/char-arrays).

## Copying

```c
void copy(char dst[], char src[]) {
    int i = 0;
    while (src[i] != '\0') {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';        // do not forget the terminator!
}
```

The destination must have room for the text **and** the terminator. If `dst` is too small, you write past its end, a bug that is the root of countless security holes.

## Comparing

Two strings are equal if they match at every position and end at the same place. Walk both with the same index. When they first differ (and a terminator counts as a character that differs from any letter), the difference decides which comes first in dictionary order:

```c
int i = 0;
while (a[i] != '\0' && a[i] == b[i]) {
    i++;
}
// now a[i] != b[i], or both are '\0'
```

If both are `'\0'` the strings are equal. Otherwise compare `a[i]` and `b[i]` (as `unsigned char`).

## Appending

To add `src` to the end of `dst`, first find where `dst` ends, then copy `src` there, including the terminator.

## Your turn

Don't use `<string.h>`. The tests give your functions buffers big enough for the result.
