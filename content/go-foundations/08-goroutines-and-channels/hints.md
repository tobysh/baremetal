In `Generate` create `out := make(chan int)`, start `go func() { defer close(out); for _, n := range nums { out <- n } }()` and `return out`.
===
`Square` has the same shape but reads with `for v := range in { out <- v * v }`. Closing `out` (with `defer close(out)`) is what lets the next stage's `range` finish.
===
`Collect` is a plain loop: `result := []int{}; for v := range in { result = append(result, v) }; return result`. It does not need a goroutine.
