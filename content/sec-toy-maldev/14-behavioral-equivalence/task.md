The helpers from the earlier lessons are supplied (`Assemble`, `Wrap`, `Pack`, `Unpack`, `LoadCapsule`, `Run`, `ExecuteBundle`). Implement:

```go
type Scenario struct {
    Caps     map[string]string
    MaxOps   int
    MaxTicks uint32
}

func TraceKey(t Trace) string
func Equivalent(a, b []byte, scenarios []Scenario) (bool, error)
```

**TraceKey** returns a canonical lowercase hex SHA-256 string of a trace. Hash this encoding: the number of outputs as 4 bytes little-endian, then each output as a 4-byte little-endian length plus its bytes; the same for the Reads list; then Ticks as 4 bytes little-endian. A nil and an empty slice must give the same key, and any two traces that differ in outputs, reads, ticks or how the strings are split must differ.

**Equivalent** runs both bundles with `ExecuteBundle` in every scenario and returns whether the outcomes match everywhere. Per scenario: both succeed with equal `TraceKey`, or both fail (error text is ignored). Return `false, nil` at the first mismatch. Return `false, error` if `scenarios` is empty or has more than 64 entries. Scenario maps must not be modified, and a scenario with an unusable budget is still an ordinary scenario (a failure for both).

Bundles that differ only in XOR key, in unreachable prefix instructions, or in how WAIT time is split are tried under various scenarios.
