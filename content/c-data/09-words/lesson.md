---
title: Words in a Sentence
summary: Split text into words by tracking whether you are inside one, and shift letters around the alphabet with wraparound.
minutes: 20
difficulty: medium
tags: strings, algorithms
id: c-data/words
mode: harness
---
Real text has structure: words separated by spaces. Counting words sounds easy and is full of edge cases (extra spaces, spaces at the ends, empty text). The trick is a small **state**: a variable that remembers whether the previous character was inside a word.

```c
int in_word = 0;
for (int i = 0; s[i] != '\0'; i++) {
    if (s[i] != ' ' && !in_word) {
        // a new word starts here
        in_word = 1;
    } else if (s[i] == ' ') {
        in_word = 0;
    }
}
```

A word starts at a non-space character whose previous character was a space (or the start). Count those starts and you count the words, however many spaces separate them. To find the longest word, count the length of the current word as you go, and reset it on each space.

## Wrapping around the alphabet

A **Caesar cipher** shifts each letter by a fixed amount: with a shift of 3, `a` becomes `d` and `y` becomes `b`. The wraparound is done with the remainder operator on the letter's position (0 to 25):

```c
int pos = c - 'a';                  // 0..25
pos = (pos + shift) % 26;           // wraps
c = 'a' + pos;
```

Careful: `%` of a negative number is negative in C, so for a negative `shift` first bring it into range with `((shift % 26) + 26) % 26`. Handle capitals with `'A'` as the base, and leave other characters alone.

You already know the string walking from [Strings Are Arrays of Characters](/learn/c-data/char-arrays).
