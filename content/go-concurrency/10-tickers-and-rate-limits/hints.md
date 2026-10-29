`RateLimited`: make `out`, then a goroutine with `defer close(out)`, `t := time.NewTicker(every)`, `defer t.Stop()`, and `for v := range in { <-t.C; out <- v }`.
===
`Poll`: create the ticker and `defer t.Stop()`. Then `for { select { case <-t.C: if check() { return true }; case <-ctx.Done(): return false } }`.
===
A `for` loop with a `select` inside it is the standard shape of a long-running goroutine.
