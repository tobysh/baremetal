`SleepCtx`: `select { case <-time.After(d): return nil; case <-ctx.Done(): return ctx.Err() }`.
===
`Ticks`: start a goroutine with `defer close(out)` and a `for i := 0; ; i++` loop. Inside, `select` between `out <- i` and `<-ctx.Done()`; return on the second.
===
`CallWithTimeout`: `ctx, cancel := context.WithTimeout(context.Background(), d)`, then `defer cancel()`, then `return f(ctx)`.
