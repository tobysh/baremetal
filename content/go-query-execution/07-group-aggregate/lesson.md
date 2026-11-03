---
title: Grouping with NULL Values
summary: Group nullable keys, distinguish COUNT(*) from COUNT(value), and compute nullable sums.
minutes: 40
difficulty: medium
id: go-query-execution/group-aggregate
---
Aggregation collapses rows into one result per key. NULL keys form a single group, but NULL values do not contribute to COUNT(value) or SUM(value). An all-NULL group has a NULL sum rather than zero. This model uses signed 64-bit integers; inputs are guaranteed not to overflow. Output follows first appearance of each key so results are deterministic without sorting.

Run the starter to inspect the public example, then implement the contract in the task. The reference implementation produces the specified results for these same inputs.
