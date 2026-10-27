---
title: Palindromes
summary: Check whether a string reads the same both ways by comparing characters from both ends, then ignore case with tolower and an unsigned char cast.
minutes: 20
difficulty: medium
tags: strings, algorithms
id: c-memory/palindromes
mode: harness
---
A **palindrome** reads the same forwards and backwards: "racecar", "noon", "level". You already know the two-index technique from the last lesson. Here you use it to *compare* instead of swap.

## The idea

Compare the first and last characters. If they differ, it is not a palindrome. If they match, move both inward and repeat. If the indices meet or cross without finding a mismatch, it is a palindrome.

```c
int starts_and_ends_alike(const char *s) {
    size_t len = strlen(s);
    if (len == 0) return 1;
    return s[0] == s[len - 1];
}
```

Extend that to a loop and you have the whole function. Note the `const`: checking a string does not change it, so the function promises it will not.

## Edge cases to decide up front

- The empty string `""` has nothing to disagree with itself, so it **is** a palindrome.
- A single character is a palindrome.
- Careful with `len - 1` when `len` is 0, because `size_t` is unsigned and wraps to a huge number. Return early for those short cases.

## Ignoring case

"Level" is not a palindrome by exact comparison, because `'L'` and `'l'` are different characters (they have different numeric codes). To ignore case, convert both to lowercase before comparing. The standard library gives you `tolower` in `<ctype.h>`:

```c
#include <ctype.h>

char c = 'Q';
char lower = (char)tolower((unsigned char)c);   // 'q'
```

## Why the cast to unsigned char?

Functions in `<ctype.h>` take an `int` that must be either a value of `unsigned char` (0 to 255) or the special constant `EOF` (usually -1). A plain `char` might be signed on your machine, so a byte above 127 (like part of an accented letter) can become a negative number. Passing a negative value other than `EOF` is undefined behaviour. Casting to `unsigned char` first makes the value safe:

```c
tolower((unsigned char)s[i])
```

It looks fussy, but it is the correct habit, and sanitiser-enabled builds can flag mistakes here.

## A shortcut with a helper

You can write the loop once and share it: the exact-match version compares `s[left]` directly; the case-insensitive one compares `tolower((unsigned char)s[left])`. Writing two small loops is fine, or write one static helper that takes a flag.

## Common mistakes

- Looping over the whole string instead of just half.
- Forgetting that `'A'` and `'a'` differ when comparing exactly. Spaces and punctuation also count as characters here; "a b a" is a palindrome but "ab a" is not.
- Calling `tolower` without the cast, or forgetting `#include <ctype.h>`.

Write both versions: one exact, one ignoring case.
