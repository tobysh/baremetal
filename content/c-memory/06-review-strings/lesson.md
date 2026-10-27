---
title: "Review: String Toolkit"
summary: A consolidation exercise. Combine pointer walking, two-ended swapping and skipping characters in three small string tools. No new ideas.
minutes: 20
difficulty: medium
kind: review
tags: strings, pointers, review
id: c-memory/review-strings
mode: harness
---
This is a **review lesson**. It introduces nothing new. Instead it asks you to pull together three things you have already done in this course, each with a small twist, so they stick.

## What you already know

- **String Length:** a C string ends at the `'\0'` byte, so you can walk a pointer until you reach it.
- **Reverse a String:** two indices, one at each end, swap and move inwards.
- **Palindromes:** compare from both ends, and use `tolower` with an `unsigned char` cast to ignore case.

## The twists

1. **Count, don't measure.** Walk the string like `strlen` does, but add one only when the character matches.
2. **Reverse only part of it.** Same swap loop as before, but the right-hand index starts at `n - 1` rather than at the end of the string.
3. **Skip what doesn't count.** A phrase palindrome ignores everything that isn't a letter or digit. Move `left` forward and `right` backward past such characters before comparing. `isalnum` from `<ctype.h>` tests a character (cast to `unsigned char` first, as with `tolower`).

Take your time. If one part feels shaky, reread the lesson it came from. The hints build up gradually.
