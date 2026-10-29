`CallWithTimeout`: `ctx, cancel := context.WithTimeout(context.Background(), d)` and `defer cancel()`. Start `go func() { s, err := f(ctx); ch <- result{s, err} }()` with `ch := make(chan result, 1)`.
===
Then `select { case r := <-ch: return r.s, r.err; case <-ctx.Done(): return "", ctx.Err() }`. If `f` finishes at the same moment as the deadline either answer is fine.
===
`Remaining`: `dl, ok := ctx.Deadline(); if !ok { return 0, false }; left := time.Until(dl); if left < 0 { left = 0 }; return left, true`.
===
`Half`: use `Remaining`. When `ok` is false return `context.WithCancel(ctx)`. Otherwise `context.WithTimeout(ctx, left/2)`.
