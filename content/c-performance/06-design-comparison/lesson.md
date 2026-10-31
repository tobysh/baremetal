---
title: Design Comparison
summary: Build a sorted array and a hash table for the same key set, prove they agree, count what each costs, and write a chooser that picks by evidence.
minutes: 45
difficulty: hard
tags: performance, design, tradeoffs, evidence
id: c-performance/design-comparison
mode: harness
---
Engineers are often asked "which is faster?" The honest answer is "for which workload, by which measure, and what did you leave out?" This lesson is a **design comparison**: two implementations of one interface, a proof they behave the same, a cost table, and a decision.

## The interface: a set of positive ints

```text
insert(k)        add k; returns 1 if it was new, 0 if already present
has(k)           1 if present
range(lo, hi)    how many keys k satisfy lo <= k <= hi
```

**Design A, sorted array** (`sa_*`): keys in ascending order. Lookups are binary searches; insert finds the slot then shifts later keys right; a range query is two binary searches and a subtraction.

**Design B, open-addressing hash table** (`hb_*`): 8192 slots, 0 means empty, a key lives at `hash_key(k)` or the next free slot (linear probing). Lookup and insert are about one probe at this load. A range query has no order to exploit, so it must **scan every slot**.

## The cost model

Each structure keeps a `cost_t` you must maintain honestly:

- `probes`: one per stored key or slot you read to compare or test. (Binary search: one per step.)
- `moves`: one per key copied while shifting for an insert.
- `init`: slots written when the structure is created (B writes all 8192; A none).

Total cost is `probes + moves + init`. Memory in the model is `4 * n` bytes for A and `4 * 8192` for B. This is an **operation-count** model. It ignores cache behaviour (A's binary search jumps around; B's probes are random accesses; a shift is sequential and cheap per element on real hardware), so a `move` is probably cheaper than a `probe` in practice. It also ignores hash quality on adversarial keys and resizing. State those limits whenever you present a result.

## What gets graded, and what cannot be

1. **Equal results**: both designs are run against a plain reference over random operation sequences.
2. **Counters are real**: A's `moves` for an insert must equal the number of larger keys, binary-search probes stay within `ceil(log2(n+1)) + 1`, B's range scan costs exactly 8192 probes, and B's lookups average a small number of probes.
3. **The chooser**: `choose_design(ops, nops, mem_limit)` runs both designs on a workload, drops any design whose model memory exceeds `mem_limit`, and returns `'A'` or `'B'` for the lower total cost (`'A'` on a tie), or `'?'` if neither fits. The tests compare its answer with an independent analytic estimate on workloads where the winner is clear (at least 3 times cheaper).

What a machine **cannot** grade is the written justification: why the break-even sits where it does, and whether the model's omissions would flip the answer on your hardware. Write that reasoning down in comments. A good one names the workload, quotes the counters, and says what was not measured.

## Surprises the numbers show

- For a handful of operations, B loses: it pays 8192 for `init` before doing anything.
- Random inserts into A are costly (about half the keys move each time); sorted-order inserts cost no moves.
- Range queries decide it for B: one range scan is worth thousands of lookups.
