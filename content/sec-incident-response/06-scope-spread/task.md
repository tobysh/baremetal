Implement `Scope(startCred string, startTime int64, events []Event, stored map[string][]string) []Item`.

An `Event` is `{Time, Actor, Action, Object}`. Actions: `"login"` (Actor, a credential, logs in to host Object) and `"read-secret"` (Actor reads credential Object). `stored[host]` lists credentials kept on that host.

`startCred` is compromised from `startTime`. An event takes effect when its actor was compromised at a time `<= event.Time`:
- `login`: the host Object becomes compromised at `event.Time`, and so does each credential in `stored[host]`.
- `read-secret`: credential Object becomes compromised at `event.Time`.

Keep the **earliest** time per item. Events in the same second are simultaneous; input order is arbitrary. Repeat until nothing changes. Return `Item{Name, Kind, Since}` with Kind `"credential"` or `"host"`, including `startCred`, sorted by Since, then Kind (`credential` before `host`), then Name. Other actions are ignored.

Public example: `svc` starts compromised at t=10; `svc` logs in to `db1` at t=20 where `key9` is stored; `key9` reads `vault` at t=30. All of those are in scope; a login by `svc` at t=5 is not.
