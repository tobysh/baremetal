---
title: "Review: Vector Toolkit"
summary: A consolidation exercise. Use your growable array together with function pointers to write map, filter and reduce. No new ideas, just practice.
minutes: 20
difficulty: medium
kind: review
tags: function-pointers, heap, data-structures, review
id: c-systems/review-vector-toolkit
mode: harness
---
This is a **review lesson**. It introduces nothing new. You have built a growable array and you have used function pointers. Here you combine them, which is how a lot of real C libraries are shaped.

## What you already know

- **Structs:** `IntVec` bundles a pointer with its `len` and `cap`.
- **The growable array:** `data[0..len)` holds the elements, and `cap` is how many fit before the next `realloc`.
- **Function pointers:** `int (*f)(int)` is a variable that holds the address of a function taking an `int` and returning an `int`. You call it like any function: `f(x)`.

## What is provided

The starter already contains working `vec_init`, `vec_push` and `vec_free`, written the way the earlier lesson asked, so you can focus on the new combination. Read them first; you may find you could have written them from memory.

## The three functions

1. **`vec_map`** replaces every element `x` with `f(x)`, in place. It is a loop and a function-pointer call.
2. **`vec_filter`** keeps only the elements for which the predicate returns non-zero, and keeps them **in the original order**. The classic way to do this in place is two indices: `read` visits every element, `write` marks where the next kept element goes. When `read` finds one to keep, copy it to `write` and advance `write`. At the end, `len = write`. It never needs more room, so it never allocates.
3. **`vec_reduce`** folds the whole vector into one value. Start with an accumulator, and for each element set `acc = f(acc, x)`. Summing is one use, finding a maximum is another; the caller decides by passing a different function.

A good habit with callbacks: write the loop first with the callback as a plain function you control, then confirm it works with a second, different callback. If it only works for one, it is probably hard-coded.
