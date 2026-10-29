`Counter`: `out := make(chan int)`, then `go func() { defer close(out); for i := 0; ; i++ { select { case out <- i: case <-ctx.Done(): return } } }()` and `return out`.
===
`Sleep`: first `if err := ctx.Err(); err != nil { return err }`. Then make `t := time.NewTimer(d)`, `defer t.Stop()`, and `select` on `t.C` (return `nil`) and `ctx.Done()` (return `ctx.Err()`).
===
`Drain`: `result := []int{}` then loop with `select { case v, ok := <-in: if !ok { return result }; result = append(result, v); case <-ctx.Done(): return result }`.
