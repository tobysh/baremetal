---
title: Regression Baseline
summary: Speed up a slow but correct function without changing what it does. Keep the baseline, change one thing, rerun the same inputs, and watch a work counter.
minutes: 25
difficulty: medium
tags: debugging, regression, baseline
id: c-debugging/regression-baseline
mode: harness
---
A fix or an optimisation is a *change*, and every change can break something that used to work. The defence is a **baseline**: a version you trust, kept next to the new one, so you can ask "do they still agree?"

## The routine

1. **Baseline first.** Keep the slow, obviously correct version. Do not delete it, and do not "improve" it while you are comparing.
2. **Fix a set of inputs.** Use the same inputs for every run, including the boring edges: empty, one element, all equal, negative values.
3. **Change one thing.** If you rewrite the algorithm and the output format together, a mismatch could come from either.
4. **Rerun the same inputs** and compare results, not impressions.

## Measure work, not time

Wall-clock time is noisy: another process, a cold cache or a slow machine all move it. A deterministic **work counter** is better. Add one to a global each time the code inspects an element, and the number is the same on every run and every machine.

```c
long bm_work;          // reset before a call, read after it
...
bm_work++;             // once per element inspected
```

## The tempting version

`distinct_baseline` counts the different values in an array by comparing each element with every earlier one. It is correct and slow: the work grows with the *square* of `n`.

`distinct_fast` is a quick rewrite that remembers the values it has seen in a small table, valid because values lie in `-100..100`. It is fast, and it quietly behaves differently from the baseline. A comparison on a few ordinary inputs may not show it. Look at the edges: what does it return for an empty array, and what does it do with `-3` and `3`?

## Your turn

Fix `distinct_fast` so it matches `distinct_baseline` on every input, and keep its work within the budget. The baseline is the specification here: if the baseline says something odd, so does the fast version.
