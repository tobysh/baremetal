---
title: String Length
summary: A C string is just an array of chars ending in a zero byte. Learn the null terminator and write your own strlen by walking a pointer along the string.
minutes: 15
difficulty: easy
tags: strings, pointers
id: c-memory/string-length
mode: harness
---
C has no special string type. A **string** is an array of `char` (one-byte characters) whose end is marked by a special byte with the value 0, written `'\0'` and called the **null terminator**.

```c
char word[] = "cat";
```

In memory that is four bytes, not three:

| index | 0 | 1 | 2 | 3 |
|-------|-----|-----|-----|------|
| value | `'c'` | `'a'` | `'t'` | `'\0'` |

The terminator is how every string function knows where to stop. Nothing stores the length; you find the end by walking until you hit the zero.

## Walking a string

```c
const char *s = "cat";
int i = 0;
while (s[i] != '\0') {
    printf("%c\n", s[i]);
    i++;
}
```

Notice `const char *s`. A string passed to a function arrives as a pointer to its first character (the same decay you saw with arrays). `const` promises we will not change the characters.

You can also move the pointer itself:

```c
const char *p = s;
while (*p != '\0') {
    p++;            // step to the next character
}
// p now points at the terminator
```

Because `'\0'` is 0, which counts as false in a condition, `while (*p)` is the same as `while (*p != '\0')`. Both styles are fine; use the clearer one while learning.

## Length does not include the terminator

The **length** of `"cat"` is 3, even though it occupies 4 bytes. The standard library has `strlen` (in `<string.h>`) that does exactly this walk. Today you will write your own, which is a rite of passage for C programmers.

The return type is `size_t`, an unsigned integer type made for sizes and counts.

## Common mistakes

- Forgetting the terminator when sizing a buffer: `char buf[3] = "cat";` leaves no room for `'\0'`, so it is not a valid string.
- Confusing `'a'` (a single character, in single quotes) with `"a"` (a string, two bytes: `'a'` and `'\0'`).
- Calling `strlen` in a loop condition on a long string. It walks the whole string every time.
- Running past the end because the string was never terminated. Without the zero, your loop keeps reading whatever bytes come next in memory, which is undefined behaviour.

Write `my_strlen` without using `strlen`.
