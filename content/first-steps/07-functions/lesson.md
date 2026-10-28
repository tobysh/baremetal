---
title: Functions
summary: Package instructions into reusable functions that take inputs and return a result.
minutes: 15
difficulty: easy
tags: functions
id: first-steps/functions
mode: harness
---
A **function** is a named, reusable recipe. You give it inputs (**parameters**), it does some work, and it hands back a result (the **return value**).

```c
int triple(int n) {
    return n * 3;
}
```

Reading the first line from left to right:

- `int` — the type of value this function returns
- `triple` — its name
- `(int n)` — it takes one input, an `int`, which it calls `n`

Now anywhere else you can **call** it:

```c
int x = triple(4);   // x is 12
printf("%d\n", triple(10));  // prints 30
```

Functions can take several parameters, separated by commas:

```c
int area(int width, int height) {
    return width * height;
}
```

## Why bother?

Functions let you name an idea once and reuse it everywhere. They also let you **test** pieces of a program on their own — which is exactly what happens when you press **Test** from now on. Our hidden tests call your functions with different inputs and check the results.

You can still write a `main` to try your functions out with **Run**. The tests ignore it.
