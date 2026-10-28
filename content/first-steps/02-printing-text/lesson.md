---
title: Printing text
summary: Print several lines, and use escape sequences to show tabs, quotes and backslashes.
minutes: 10
difficulty: easy
tags: basics, output
id: first-steps/printing-text
mode: stdout
---
In [Your first program](/learn/first-steps/hello-world) you printed one line. A program can print as many as it likes: each `printf` prints exactly what is inside its quotes, and the program runs the lines **in order, top to bottom**.

```c
printf("First line\n");
printf("Second line\n");
```

Without the `\n`, the next `printf` carries on *on the same line*:

```c
printf("Hello, ");
printf("world!\n");   // prints: Hello, world!
```

## Escape sequences

Inside a string, the backslash `\` is special. It starts an **escape sequence**: a two-character code that stands for one character you can't easily type.

| Write | You get |
|---|---|
| `\n` | a new line |
| `\t` | a tab (a wide gap, handy for columns) |
| `\"` | a double quote (otherwise `"` would end the string) |
| `\\` | one backslash |

```c
printf("She said \"hi\"\n");   // She said "hi"
printf("C:\\temp\n");          // C:\temp
```

## A note on spelling

The computer is exact. `Printf`, `printf ` and `print` are different things, and a missing quote or semicolon stops the compiler. If the output looks almost right, compare it with the target character by character.
