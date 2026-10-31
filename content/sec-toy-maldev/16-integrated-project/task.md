Implement the five checkpoint functions; the types and earlier helpers are supplied.

```go
type Step struct {
    Op    string // "emit", "emit-sym", "read", "wait" or "entry"
    Arg   string
    Ticks uint16
}
type Limits struct {
    MaxOps   int
    MaxTicks uint32
}
type Rules struct {
    AllowedReads map[string]bool
    MaxTicks     uint32
    Forbidden    []string
}
type Host struct{ Files map[string]string }
type Report struct {
    Trace    Trace
    Findings []string
}
```

**Checkpoint 1: `Resolve(steps []Step, cfg map[string]string) ([]Instruction, int, error)`**
`emit` emits `Arg`; `emit-sym` emits `cfg[Arg]`; `read` reads key `Arg` (non-empty); `wait` waits `Ticks` (non-zero); `entry` emits nothing and marks the entry point (at most one). Append the final HALT. The int is the entry's *instruction index* (0 when there is no marker). Error for an unknown Op, a missing symbol, any `cfg` key that no `emit-sym` used, a text longer than 255 bytes, or more than 64 instructions including HALT. Do not modify `cfg`.

**Checkpoint 2: `Build(steps, cfg, key) ([]byte, error)`**
Resolve, assemble, wrap with the entry converted to a *byte offset*, and pack with `key`.

**Checkpoint 3: `Detect(tr Trace, rules Rules) []string`**
Findings, deduplicated and sorted with `sort.Strings`, never nil: `read:<key>` for each read key not allowed; `ticks:<n>` when `tr.Ticks > rules.MaxTicks`; `output:<marker>` for each non-empty Forbidden marker that is a substring of any output.

**Checkpoint 4: `Deploy(h *Host, journal *[]string, name string, bundle []byte, tr Trace) error` and `Teardown(h *Host, journal *[]string)`**
`name` is 1 to 32 characters of `a-z`, `0-9`, `-`. Deploy creates `name/payload.bin` (the bundle bytes as a string) and then `name/trace.log` (the outputs joined by `"\n"`). It refuses to overwrite an existing entry. Before each write it first confirms the name is free, then appends the path to the journal, then writes. A refusal leaves already-created entries in the journal and never journals a path it did not create. Teardown deletes the journalled paths newest first and empties the journal; calling it again is harmless. A nil Files map must be created by Deploy.

**Checkpoint 5: `RunProject(h *Host, name string, steps []Step, cfg map[string]string, key byte, caps map[string]string, lim Limits, rules Rules) (Report, error)`**
Build, Deploy, read the payload back out of the host and execute it with `ExecuteBundle(bundle, caps, lim.MaxOps, lim.MaxTicks)`, then Detect on the trace, then Teardown. Teardown must run on every path, including when a stage fails or the detector finds something; findings are not errors. On error return `Report{}` and the error. When the function returns, `h` must hold exactly the entries it held before the call.
