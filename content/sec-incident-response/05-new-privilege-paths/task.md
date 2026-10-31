Implement `Escalations(before, after []Edge, audit []Audit, target string) ([]string, []Enabler)`.

An `Edge` is `{From, To, Kind}`. Only kinds `MemberOf` and `HasRole` count. A node reaches the target if a directed path over counting edges leads to it; the target itself is not in the reach set.

- First result: nodes that reach `target` in `after` but not in `before`, sorted.
- Second result: counting edges present in `after` but not `before` (deduplicated), whose `From` is newly privileged and whose `To` is the target or reaches it in `after`. Sort by From, To, Kind.

Each `Enabler{Edge, Actor, Time, Attributed}` takes the **latest** `Audit` entry (by `Time`) with `Action == "add"` that matches the edge's From, To and Kind exactly. With none, `Attributed` is false and Actor empty, Time 0.

Public example: before `{ann MemberOf ops}, {ops MemberOf admins}`; after adds `{tmp MemberOf ops}`; `tmp` is newly privileged and that edge is the enabler.
