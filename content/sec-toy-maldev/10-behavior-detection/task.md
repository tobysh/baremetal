Keep the supplied `Execute` and implement `Detect` and `ParseReport`.

```go
type Event struct {
	Kind  string // "read", "emit" or "wait"
	Text  string
	Ticks uint32
}
type Finding struct {
	ID       string // "secret-read", "periodic-beacon" or "delayed-start"
	Index    int    // event index
	Severity string // "high", "medium" or "low"
}
type Policy struct {
	DelayTicks uint32
	Heartbeats map[string]bool // emit texts expected to repeat
}

func Detect(trace []Event, p Policy) ([]Finding, error)
func ParseReport(data []byte) ([]Finding, error)
```

`Detect`

- An event with any other Kind is an error (`nil, error`). Inputs are not modified. With no findings return an empty non-nil slice.
- `secret-read` / `high`: one finding per `read` event whose Text starts with `secret/`. Index is that event.
- `periodic-beacon` / `medium`: for each distinct emit text not in `p.Heartbeats`, list its emit events in order. The gap between consecutive ones is the sum of the Ticks of all `wait` events between them. If some three consecutive same-text emits `e_j, e_j+1, e_j+2` have gap(j) == gap(j+1) and the gap is greater than zero, report the smallest such j, with Index of `e_j`. At most one finding per text. Sum in 64 bits.
- `delayed-start` / `low`: let f be the first event that is not a `wait`. If it exists and the Ticks of the waits before it sum to more than `p.DelayTicks`, report Index f. Traces made only of waits produce no finding.
- Sort by Index, then by ID.

`ParseReport`

- Input is one JSON object `{"schema":N,"findings":[{"id":..,"index":..,"severity":..}]}` and nothing after it. Reject unknown fields, a missing `schema` or `findings`, schemas other than 1 and 2, unknown IDs, negative indexes, and more than 64 findings.
- Schema 1: findings must **not** have a `severity`; fill it in from the ID. Schema 2: `severity` is required and must equal the ID's severity.
- Return findings in the given order. Return `nil, error` for any violation.

Example: `Execute` of `READ secret/lab; EMIT hi; HALT` with that key present gives events `read secret/lab`, `emit hi` and Detect reports `secret-read` at index 0.
