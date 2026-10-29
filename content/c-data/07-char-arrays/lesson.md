---
title: Strings Are Arrays of Characters
summary: See text as a char array that ends in a zero byte, measure it, count things in it and change letters in place.
minutes: 15
difficulty: easy
tags: strings, arrays
id: c-data/char-arrays
mode: harness
---
C has no separate text type. A **string** is an array of `char` whose last element is the special character `'\0'` (the number 0), called the **terminator**. It marks where the text ends.

```c
char word[] = "cat";
```

is the same as

```c
char word[4] = {'c', 'a', 't', '\0'};
```

Four boxes for three letters: the terminator needs its own. `word[0]` is `'c'` (see [Characters](/learn/first-steps/characters)), and `word[3]` is `'\0'`.

Unlike an `int` array, there is no length parameter. The terminator *is* the length: walk the array until you hit `'\0'`.

```c
int i = 0;
while (s[i] != '\0') {
    i++;
}
// i is now the length
```

`printf("%s", word)` does exactly this walk to know where to stop. That also means a string without a terminator is a bug: functions will keep reading past the end.

## Characters are small numbers

`'a'` is just a number (97 in ASCII), so you can compare and calculate with characters: `c >= 'a' && c <= 'z'` tests for a lowercase letter, and `c - 'a' + 'A'` turns a lowercase letter into the uppercase one. The header `<ctype.h>` offers helpers like `isdigit(c)` and `toupper(c)`; cast the argument to `unsigned char` when you call them, as in `toupper((unsigned char)c)`.

## Your turn

Write three functions. Don't use `strlen` or other string library functions, since walking the string yourself is the lesson.
