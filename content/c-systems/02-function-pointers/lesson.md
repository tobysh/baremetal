---
title: Function Pointers and Callbacks
summary: Pass a function to another function. Write map, count_if and a sort that takes its comparison as an argument, the same trick qsort uses.
minutes: 25
difficulty: medium
tags: function-pointers, callbacks
id: c-systems/function-pointers
mode: harness
---
Functions live in memory too, so they have addresses. A **function pointer** is a variable that holds the address of a function, which means you can hand a function to *another* function and let it decide when to call it. The function you hand over is called a **callback**.

Think of a town crier. He knows how to walk the streets and shout, but *what* he shouts is written on a scroll you give him. Same crier, different scroll, different message.

## The syntax

It looks odd the first time. A pointer to a function taking an `int` and returning an `int`:

```c
int (*f)(int);
```

Read it from the middle: `f` is a pointer (`*f`) to a function that takes an `int` and returns an `int`. The parentheses around `*f` matter; without them you would declare a function that returns an `int *`.

Point it at a real function and call it like any other:

```c
int double_it(int x) { return x * 2; }

int (*f)(int) = double_it;     // no & needed, the name is the address
printf("%d\n", f(21));         // 42
```

## Taking a callback

```c
void apply(int *a, int n, int (*f)(int)) {
    for (int i = 0; i < n; i++) {
        a[i] = f(a[i]);
    }
}

apply(numbers, 5, double_it);
```

`apply` does not know or care what `f` does. That is the point: the looping is written once, the behaviour is plugged in.

## How qsort works

The standard library's `qsort` sorts an array of anything. It cannot know how to compare your things, so it asks you for a comparison callback that returns:

- a **negative** number if the first item should come before the second,
- **zero** if they are equal,
- a **positive** number if the first should come after.

```c
int ascending(int a, int b) { return a - b; }
int descending(int a, int b) { return b - a; }
```

Swap the callback and the same sort runs the other way round. Be careful with `a - b` on huge values since it can overflow; here the numbers are small.

## Your turn

Write three tools, each taking a callback:

1. `map_ints` transforms every element in place.
2. `count_if` counts the elements a test function approves of.
3. `sort_ints` sorts using a comparison callback. Any simple sort will do; insertion sort is a friendly one.

The tests supply their own callbacks.
