---
title: Reverse a String
summary: Reverse a string in place with the two-index technique, swapping from both ends toward the middle. Practice pointers, indices and the null terminator.
minutes: 20
difficulty: medium
tags: strings, pointers, arrays
id: c-memory/reverse-string
mode: harness
---
Reversing "hello" into "olleh" sounds like it needs a second string, but in C you can do it **in place**: rearrange the characters inside the same array, using no extra memory.

## The two-index technique

Put one index at the start and one at the end. Swap the two characters, then move both indices inward. Stop when they meet or cross.

```
h e l l o       left = 0, right = 4   swap h and o
o e l l h       left = 1, right = 3   swap e and l
o l l e h       left = 2, right = 2   they met: done
```

The middle character of an odd-length string stays put, and that is correct. Every swap fixes two positions at once, so you only need about half the work.

In code:

```c
void reverse_demo(char *s, size_t len) {
    size_t left = 0;
    size_t right = len - 1;
    while (left < right) {
        char tmp = s[left];
        s[left] = s[right];
        s[right] = tmp;
        left++;
        right--;
    }
}
```

This is the same temporary-variable trick as `swap` from lesson 1.

## Find the end first

The function only gets a `char *`. To know where `right` should start, find the length by walking to the `'\0'` (your `my_strlen`, or `strlen` from `<string.h>`). The last real character is at index `len - 1`.

## Careful with empty strings and unsigned numbers

`size_t` is unsigned: it cannot go below zero. If `len` is 0, then `len - 1` does not give -1; it wraps around to a gigantic number. Handle the empty string before subtracting, or use `if (len < 2) return;` since strings of length 0 or 1 are already their own reverse.

## Strings must be writable

```c
char good[] = "hello";     // a writable array holding a copy
const char *bad = "hello"; // points at a string literal
```

The text `"hello"` written directly in your code is a **string literal**, and it lives in read-only memory. Trying to modify it (`bad[0] = 'j';`) is undefined behaviour and usually crashes. To get a string you can change, copy it into an array with `char good[] = "hello";`. That is why this function takes `char *` and not `const char *`: it promises nothing about leaving the text alone.

## Common mistakes

- Swapping across the whole length instead of stopping at the middle. You would reverse it, then un-reverse it, ending where you started.
- Using `<=` instead of `<` in the loop, which is harmless here but sloppy.
- Moving the terminator. The `'\0'` stays at index `len`; only the characters before it move.

Write `reverse`, which turns the string into its mirror image.
