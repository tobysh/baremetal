Stdin lines (any order, at most 100 nodes): `leaf <id> <cost> [mitigated]`, `and <id> <child> <child> ...`, `or <id> <child> <child> ...` and exactly one `root <id>`. Costs are integers from 0 to 100000. Every node is the child of at most one parent and there are no cycles. Print three lines:

`unmitigated cost: <N>` (mitigated flags ignored)
`mitigated cost: <N>` or `mitigated cost: infeasible`
`cheapest path: <leaf ids separated by spaces>` for the mitigated tree, in left-to-right order of the tree as listed in the node lines (children in the order written), or `cheapest path: none` when infeasible.

An OR node chooses its first child with the lowest cost.
