---
title: Your First Struct
summary: Group related values into one struct, give the type a name with typedef, and pass, return and nest structs. Model clock times as hours and minutes.
minutes: 15
difficulty: easy
tags: structs
id: c-data/first-struct
mode: harness
---
An array holds many values of the **same** type. A **struct** holds a few values of **different** types, each with its own name. A time of day is two numbers that belong together:

```c
typedef struct {
    int hours;
    int minutes;
} Time;
```

`typedef` gives the struct type a short name, so `Time` works like `int` does. Create one, read and write its **fields** with a dot:

```c
Time t = {9, 30};
printf("%d:%02d\n", t.hours, t.minutes);   // 9:30
t.minutes = 45;
```

(`%02d` pads with a zero to two digits.) You can also set fields by name: `Time t = {.hours = 9, .minutes = 30};`.

## Structs are passed and returned by value

Pass a struct to a function and the function gets a **copy**, like an `int`. Changing the copy does not change the caller's. To produce a new value, build one and `return` it:

```c
Time time_make(int h, int m) {
    Time t = {h, m};
    return t;
}
```

This is the opposite of arrays, where the function works on the original. (Later courses show how to let a function modify a struct: that needs a pointer.) You can compare fields but not whole structs with `==`.

## Normalising

Fields often have rules: `minutes` should be 0 to 59. When you add two times, 45 + 30 minutes is 75, which must become 1 hour and 15 minutes. Division and remainder from [Division and Remainder](/learn/first-steps/division-and-remainder) do it: `total / 60` and `total % 60`.

## Your turn

`Time` is defined in the starter. Hours do not wrap at 24.
