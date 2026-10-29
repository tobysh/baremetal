Fields you need: `wg sync.WaitGroup`, `cancel context.CancelFunc`, `once sync.Once`, `err error`.
===
`WithContext`: `ctx, cancel := context.WithCancel(ctx)`, then `return &Group{cancel: cancel}, ctx`.
===
`Go`: `g.wg.Add(1)`, then a goroutine that does `defer g.wg.Done()`, calls `f()`, and on a non-nil error runs `g.once.Do(func() { g.err = err; g.cancel() })`.
===
`Wait`: `g.wg.Wait()`, then `g.cancel()` (guard against a nil cancel if the group was not made with `WithContext`), then `return g.err`.
