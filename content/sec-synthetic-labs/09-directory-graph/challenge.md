**Track challenge: cut every route to Domain Admin.**

The final lesson finds one shortest path. Defenders need a stronger answer: which smallest set of controllable relationships would remove **all** paths from the starting identity to the privileged group? Removing an edge on the first path may leave another path intact.

Keep the supplied `AdminPath` implementation and add:

```go
func RemediationCut(raw, start, goal string) ([]Edge, error)
```

Use the same Directory schema, graph validation and four traversable edge kinds as the lesson. Only `GenericAll`, `AdminTo` and `HasSession` edges can be removed in this lab. `MemberOf` is traversable but fixed. Other kinds remain ignored. Duplicate identical edges represent one relationship and must be removed together.

Return the **fewest distinct removable edges** whose removal makes goal unreachable from start. Sort the result by From, then To, then Kind, using Go string ordering. If multiple smallest cuts exist, return the lexicographically smallest sorted list using that edge ordering. Node labels are never used for comparison.

- Reject invalid graphs exactly as `AdminPath` does.
- Reject inputs with more than 16 distinct removable edges, including edges in disconnected components. This small bound makes exhaustive subset search reasonable.
- Return an empty slice and no error if no route exists already.
- Return an error if no cut exists, including start == goal or a route consisting entirely of MemberOf edges.
- Leave the input untouched. No environment changes or network calls are involved.

Example: guest→ops (MemberOf), ops→host (AdminTo), host→admin (HasSession), admin→da (MemberOf). Either removable edge cuts the route. Lexical ordering chooses `{From:"host", To:"admin", Kind:"HasSession"}` before the ops edge. Two independent routes may require two removals.

**Why this is harder:** shortest-path search answers whether one route exists. This task quantifies over every route and optimizes a set of changes. A valid-looking single-path fix can still leave privilege reachable. For at most 16 relationships, try combinations in increasing size and lexical order, using reachability as the predicate. In larger graphs this connects to minimum-cut algorithms.

**What the tests establish:** minimality, deterministic tie breaking, cycles, disconnected components, duplicate relationships, fixed membership-only routes and validation errors. Passing demonstrates analysis of the fictional model; a real remediation plan still needs operational review of each relationship.
