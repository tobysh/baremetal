---
title: Array of Structs or Not
summary: Convert a hot loop from an array of structs to separate arrays, check the results are identical, and count the misses saved. Then see when the layout loses.
minutes: 30
difficulty: medium
tags: performance, layout, soa, cache
id: c-performance/array-of-structs-or-not
mode: harness
---
**Layout** is how fields sit in memory. The same data can be an array of structs (AoS) or one array per field (struct of arrays, SoA).

## The workload

A particle has 8 numbers and an 8-int tag, 64 bytes, exactly one cache line:

```c
typedef struct { int id, x, y, z, vx, vy, vz, mass; int tag[8]; } particle;
```

The hot loop only moves along one axis: `x += vx` for every particle. In AoS form each particle's `x` and `vx` share one line, but the loop also drags in the other 56 bytes it does not need. 128 particles, 128 lines, **128 misses** and 384 accesses (two loads and a store per particle).

In SoA form `x` values sit side by side (16 per line) and so do `vx` values. The same loop reads 8 lines of `x` and 8 of `vx`: **16 misses**, same 384 accesses.

## Equal results first

A layout change is a refactor, and a refactor must not change results. The test applies several steps and compares with a reference computed from the original values with plain arithmetic, so a fast loop that does the wrong thing cannot pass.

## The honest tradeoff

SoA is not "better". Suppose a different loop reads **all eight numbers of one particle** (say, to print it). AoS: one line, one miss. SoA with eight separate arrays: eight different lines, up to eight misses. Layout should follow the *access pattern*: fields that are used together belong together. A mixed program may split only the hot fields out (hot/cold splitting).

Limits of this lesson's evidence: the model has no prefetcher (real hardware streams sequential data well, narrowing the gap) and no SIMD (real SoA loops often vectorise, widening it). The counts tell you which layout does less cache work for this loop, not by how much it is faster on your machine.

## Your turn

`soa_load()` copies `x` and `vx` from the particle array into `sx` and `svx`. Make `advance_x_soa()` do the same step as `advance_x_aos()` using only those two arrays.
