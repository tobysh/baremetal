---
title: "Review: Pointers and the Heap"
summary: A consolidation exercise. Combine pointer walking, out-parameters and malloc in three small functions. No new ideas.
minutes: 25
difficulty: medium
kind: review
tags: pointers, heap
id: c-memory/review-pointers-heap
mode: harness
---
This is a **review lesson**. It introduces nothing new. It pulls together ideas you have already used in this track, each with a small twist.

## What to remember

- A pointer can step through an array: `p + 1` is the next element, and `end - begin` counts elements.
- To give a caller extra results, write through an **out-parameter** (a pointer the caller supplies): `*out = value;`.
- `malloc` reserves bytes you must size yourself (`n * sizeof *p`, or `strlen(s) + 1` for text), may return `NULL`, and must be matched by exactly one `free` in the caller.
- Whoever receives a heap pointer from a function owns it.

## Your turn

Write three functions. Each uses two or three of the ideas above and nothing more.
