---
title: Reading a number
summary: Use scanf to read an integer from input and use it in your program.
minutes: 15
difficulty: easy
tags: input, variables
id: first-steps/reading-a-number
mode: stdout
---
Until now, the values in your programs came from the code itself. **Input** lets the same program work with different values each time you run it.

`printf` writes output. `scanf` reads input. Both are declared in `<stdio.h>`.

## Reading one integer

```c
#include <stdio.h>

int main(void) {
    int age;
    if (scanf("%d", &age) != 1) {
        return 1;
    }
    printf("Next year you'll be %d\n", age + 1);
    return 0;
}
```

If the input is `20`, this prints `Next year you'll be 21`.

Take the reading step apart:

- `int age;` creates a variable to hold a whole number.
- `"%d"` tells `scanf` to read a decimal integer, just as `%d` in `printf` prints an integer.
- `&age` tells `scanf` **where to store** that number. The `&` means "the address of". You'll explore addresses in the *Pointers & Memory* course; for now, remember to put `&` before an integer variable when reading into it.
- `scanf` returns the number of values it successfully read. Here we expect `1`. If it returns anything else, `return 1;` stops the program before it uses a variable that hasn't received a value.

Unlike `printf("%d", age)`, which uses the value already in `age`, `scanf("%d", &age)` puts a new value into `age`.

## Supplying input in the workspace

Type a number into the **input** box under the editor, then press **Run**. The workspace sends that text to your program. For this lesson's exercise, try `4`, then run again with `10`.

`scanf` waits for input when none is available yet. Give the program its input before running it. You don't need to print a question or prompt: the exercise checks only the requested output.

Once a number is read, you can calculate with it or use it in an `if` condition just like any other integer variable.
