---
title: Characters and text
summary: Store a single letter in a char, see that letters are really numbers, and print text with %s.
minutes: 15
difficulty: easy
tags: types, text
id: first-steps/characters
mode: stdout
---
A **`char`** holds one character, written in *single* quotes:

```c
char letter = 'h';
printf("%c\n", letter);    // h        (%c prints it as a character)
printf("%d\n", letter);    // 104      (%d prints the number inside)
```

The second line shows the secret: the computer stores every character as a number, using the **ASCII** table. `'A'` is 65, `'B'` is 66, ... `'a'` is 97, `'b'` is 98, `'0'` is 48. A `char` is just a small integer, so you can do arithmetic on it:

```c
printf("%c\n", letter + 1);        // i   (104 + 1 is 105, which is 'i')
printf("%c\n", letter - 'a' + 'A'); // H   (shift from the lower-case row to the upper-case row)
```

The lower-case letters are in alphabetical order, the capitals are too, and the two rows are always the same distance apart. That is the trick behind every simple cipher.

## Text in double quotes

Whole words are in **double** quotes and are called **strings**. `printf` prints a string with `%s`:

```c
printf("word: %s\n", "hello");
```

Single versus double quotes matters: `'a'` is one character (a number), `"a"` is a string. You will see how strings are stored in memory later, in the C Memory course.
