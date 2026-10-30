Implement:

```go
type Host struct{ Files, Keys, Tasks map[string]string }
type Step struct{ Kind, Name, Value string } // Kind: "file", "key" or "task"
type Entry struct {
    Kind, Name string
    Existed    bool
    Old        string
}
type Journal struct{ Entries []Entry }

func Apply(h *Host, j *Journal, steps []Step, crashAt int) (crashed bool, err error)
func Recover(h *Host, j *Journal) error
func Audit(h, baseline *Host) []string
```

**Apply** sets `Value` under `Name` in the map selected by `Kind`. Validate before touching anything: `h` and `j` non-nil, the journal empty (otherwise the caller must Recover first), 1 to 32 steps, Kind one of the three, Name 1 to 64 bytes, Value at most 256 bytes. A nil map in `h` should be created. Any invalid input returns `false, error` with no change to `h` or `j`.

For each step `i`, in order: first append an Entry recording whether the name existed and its old value, and only then write the value. `crashAt` simulates an interruption and numbers the points of the lifecycle: `2*i` is "before step i is journaled", `2*i+1` is "journaled but not yet written", and `2*len(steps)` is "everything written". If `0 <= crashAt <= 2*len(steps)`, stop exactly at that point, leave `h` and `j` as they are and return `true, nil`. Any other value (use -1) runs to the end and returns `false, nil`. A completed run leaves its journal in place for teardown.

**Recover** undoes the journal from the newest entry to the oldest: restore the old value if the name existed, otherwise delete it. Then empty the journal and return nil. Validate every entry (known Kind) before undoing anything; on a bad entry return an error and leave `h` and `j` untouched. Calling Recover again, or on an empty journal, changes nothing.

**Audit** compares `h` with `baseline` and returns one string per difference, formatted `files/name added`, `files/name removed` or `files/name changed` (also `keys/` and `tasks/`), sorted with `sort.Strings`. A clean host returns an empty, non-nil slice. Treat nil maps as empty.

Step lists with names that collide with existing state, and interruptions at every point, are tried.
