`Merge`: make `out`, `wg.Add(len(chans))`, and start one goroutine per input that does `defer wg.Done()` and `for v := range c { out <- v }`.
===
Then start one more goroutine: `wg.Wait(); close(out)`. Return `out` immediately. With zero inputs `Wait` returns at once and `out` closes.
===
`FanOut`: create `outs := make([]chan int, workers)` (convert to `<-chan int` for the result). For each worker start a goroutine that does `defer close(o)` and `for v := range in { o <- f(v) }`.
===
Capture the worker's own channel in the closure (`o := outs[i]` is fine, and loop variables are per-iteration since Go 1.22).
