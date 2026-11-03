---
title: Capstone: Choose a Query Plan
summary: Choose scan and join strategies with a deterministic cost model and explicit tie-breaking.
minutes: 40
difficulty: hard
id: go-query-execution/cost-planner
---
A planner compares estimates, not elapsed time. Here each unit represents one modeled row access or comparison. The model deliberately omits disk locality, skew and memory limits; a chosen plan is optimal only among the supplied candidates under these estimates. Cardinality estimates are inputs, not observations. A real optimizer must measure estimate error as well as execution work.

Run the starter to inspect the public example, then implement the contract in the task. The reference implementation produces the specified results for these same inputs.
