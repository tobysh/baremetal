`RunAll`: declare `var wg sync.WaitGroup`, then for each task do `wg.Add(1)` and `go func() { defer wg.Done(); task() }()`. Finish with `wg.Wait()`.
===
Do not call the task directly (that would run it in your goroutine, one at a time). It must be inside a `go func() { ... }()`.
===
`Squares`: `out := make([]int, n)` (make panics for a negative n, so handle `n <= 0` first). Each goroutine writes only `out[i]`, so no lock is needed. Wait before returning.
