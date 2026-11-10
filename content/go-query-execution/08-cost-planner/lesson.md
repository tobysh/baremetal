---
title: "Capstone: Choose a Query Plan"
summary: Choose scan and join strategies with a deterministic cost model and explicit tie-breaking.
minutes: 40
difficulty: hard
id: go-query-execution/cost-planner
---
You now own three ways to join (nested loop, hash, sort-merge) and two ways to read a table (scan, index range). None of them is best everywhere. The nested loop wins on tiny inputs, the hash join on big unsorted ones, the merge join when the data already arrives sorted, and an index lookup when the other side is huge and each row has few matches. A **planner** picks among them *before* running anything, by comparing **estimated costs**.

## Cost is an estimate, not a measurement

Each unit of cost stands for one modelled row access or comparison. The planner never times anything. It is handed **statistics** (`L` and `R` row counts, `Matches` per probe, whether inputs are sorted or indexed) and turns them into numbers. Real optimisers must also guess those statistics, and a wrong guess gives a wrong plan; here the statistics are inputs, so the exercise is about the arithmetic and the decision rule.

| Candidate | Cost | Why |
|---|---|---|
| `nested` | `L*R` | every left row rescans the right side |
| `hash` | `3*(L+R)` | one pass each side, with a constant for hashing and building |
| `merge` | `L+R`, plus `4*L` if left is unsorted, plus `4*R` if right is unsorted | one pass each, but sorting must be paid for |
| `index` | `L*(2+Matches)` | one lookup (2 units) per left row plus its matches; only if the right side is indexed |

## Worked example

`L=1000, R=2000`, nothing sorted, right side indexed, `Matches=1`:

```text
nested   1000 * 2000                = 2,000,000
hash     3 * (1000 + 2000)          =     9,000
merge    3000 + 4*1000 + 4*2000     =    15,000
index    1000 * (2 + 1)             =     3,000   <- cheapest
```

The plan is `index`, cost 3000. If both inputs were already sorted, merge would cost just 3000 and tie with the index; the tie rule decides.

## Ties and determinism

Equal costs are common (empty tables make everything 0), so the choice needs a fixed rule or the same query would plan differently from run to run. The order is **nested, hash, merge, index**: the first candidate with the minimum cost wins. In code, replace the current best only when a candidate is *strictly* smaller. Writing `<=` quietly reverses the tie order.

## Access path is separate

Choosing how to read the left table is an independent decision here: use `index` if `IndexLeft` is set and `2+Selected < L` (the lookup plus the selected rows beat reading all `L`), otherwise `scan`. It does not change the join cost in this model.

## Pitfalls

- The index join is a candidate only when `IndexRight` is true, even if its formula would be cheapest.
- Counts reach one million, so `L*R` reaches 10^12 and overflows a 32-bit `int`. Use `int64`, and keep fields as `int64` rather than converting back and forth.
- Compute `merge` with both sort penalties added independently; sorted inputs add nothing.
- A plan is optimal only among the candidates and under the model, never in an absolute sense.
