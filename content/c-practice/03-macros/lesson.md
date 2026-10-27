---
title: The Preprocessor and Macros
summary: Learn what #define really does, why unbracketed macros give wrong answers, and write five small macros that behave themselves.
minutes: 30
difficulty: medium
tags: preprocessor, macros
id: c-practice/macros
mode: harness
---
Before your C compiler sees your code, a helper called the **preprocessor** runs over it. It knows nothing about C. It only does text substitution: copy this, paste that. Every line starting with `#` is an instruction to it (`#include`, `#define`, `#ifdef`).

## Constants and macros

```c
#define MAX_USERS 100
```

Everywhere the preprocessor sees `MAX_USERS`, it pastes `100`. A **function-like macro** takes arguments:

```c
#define DOUBLE(x) x + x
```

`DOUBLE(4)` becomes the text `4 + 4`. Looks fine. But what does `DOUBLE(4) * 3` become? `4 + 4 * 3`, which is `16`, not `24`. The preprocessor pasted text; it did not compute `8` first.

## Pitfall 1: bracket everything

Wrap **each argument** and **the whole result** in parentheses:

```c
#define DOUBLE(x) ((x) + (x))
```

Now `DOUBLE(1 + 2)` becomes `((1 + 2) + (1 + 2))`, which is `6`. Without the inner brackets it would be `1 + 2 + 1 + 2`, which only works by luck, and `SQUARE(1 + 2)` written as `x * x` would give `1 + 2 * 1 + 2 = 5` instead of `9`.

## Pitfall 2: arguments may run twice

`SQUARE(x)` pastes `x` twice. If the argument has a side effect, like `SQUARE(i++)`, it happens twice. Macros cannot avoid this. Knowing it exists is the defence: never pass `i++` or a function call with effects to a macro. (When you need safety, write a real function.)

## Pitfall 3: hidden names

A macro that needs a temporary variable can accidentally clash with the caller's variable of the same name:

```c
#define SWAP_INT(a, b) do { int t = (a); (a) = (b); (b) = t; } while (0)
```

Try `SWAP_INT(t, u)` with the caller's own variable called `t`: it expands to `int t = (t);`, which reads the brand-new, uninitialised `t`. Choose an unusual name such as `swap_tmp_`.

## Pitfall 4: multi-statement macros

The `do { ... } while (0)` wrapper makes a multi-statement macro behave like a single statement, so `if (x) SWAP_INT(a, b); else ...` works. Without it, the `;` ends the `if` early and the `else` is a compile error. It is the standard idiom; use it.

## Two handy operators

- `#x` turns a macro argument into a string: `STRINGIFY(hello)` becomes `"hello"`. The trick is that you may need a quoted string even though you wrote no quotes.
- `sizeof(arr) / sizeof((arr)[0])` gives the number of items in a real array (not a pointer to one!).

## Your turn

The starter has five macros that each contain a classic mistake. Fix them. The tests use arguments like `1 + 2` to catch missing brackets.
