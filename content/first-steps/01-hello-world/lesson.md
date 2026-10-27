---
title: Your first program
summary: Write, compile and run your first C program, and learn what a compiler actually does.
minutes: 10
difficulty: easy
tags: basics
id: first-steps/hello-world
mode: stdout
---
A **program** is a list of instructions for a computer. Computers are fast but extremely literal: they do *exactly* what you write, in order, and nothing else.

The CPU inside your computer only understands **machine code** — numbers. Writing numbers by hand is miserable, so we write in a language humans can read, like C, and a program called a **compiler** translates it into machine code. When you press **Run** here, we compile your code with `gcc` and then run the result.

## The smallest useful C program

```c
#include <stdio.h>

int main(void) {
    printf("Hi!\n");
    return 0;
}
```

Line by line:

- `#include <stdio.h>` borrows code someone else wrote for **st**an**d**ard **i**nput and **o**utput — including `printf`, which prints text.
- `int main(void) { ... }` defines **main**, the function where every C program starts. The curly braces `{ }` hold its instructions.
- `printf("Hi!\n");` prints `Hi!`. The `\n` means "new line". The semicolon `;` ends the instruction, like a full stop ends a sentence.
- `return 0;` tells the operating system the program finished successfully.

## When things go wrong

Forget a semicolon and the compiler will refuse to build your program and print an **error**. That's normal — professional programmers see compiler errors all day. Read the *first* error, look at the line number it mentions, and fix that first.
