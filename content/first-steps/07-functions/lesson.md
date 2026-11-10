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

## What's in memory: the call stack

Every call gets its own **stack frame**: a block of memory holding that call's parameters and local variables. The frame is created when the function is called and discarded when it returns. Newest frame on top:

```c
int triple(int n) {
    int result = n * 3;
    return result;
}

int main(void) {
    int x = triple(4);
    printf("%d\n", x);
}
```

Before the call, only `main` has a frame, and `x` has no value yet. While `triple(4)` runs, a second frame sits on top of it. The argument 4 was copied into `n`:

```text
before the call     inside triple          after the return
                    triple  n = 4
                            result = 12
main  x = ?         main    x = ?          main  x = 12
```

`return result;` copies 12 back to the caller, and the `triple` frame is gone. Only then does the assignment fill `x`.

## Scope and lifetime

A name works only inside the function that declares it: that is its **scope**. The box lives only while its frame does: that is its **lifetime**. Two functions can each have a variable called `n`, and they are different boxes in different frames. Because arguments are copied, a function cannot change the caller's variable by changing its own parameter:

```c
void bump(int n) { n = n + 1; }

int main(void) {
    int n = 5;
    bump(n);
    printf("%d\n", n);   // prints 5
}
```

## Why bother?

Functions let you name an idea once and reuse it everywhere. They also let you **test** pieces of a program on their own — which is exactly what happens when you press **Test** from now on. Our hidden tests call your functions with different inputs and check the results.

You can still write a `main` to try your functions out with **Run**. The tests ignore it.
