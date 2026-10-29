The struct needs one field: `slots chan struct{}`, created with `make(chan struct{}, n)`.
===
`Acquire` is `l.slots <- struct{}{}` and `Release` is `<-l.slots`.
===
`TryAcquire` and `AcquireCtx` are the two `select` snippets from the lesson with `l.slots` as the channel.
===
`Do`: `l.Acquire()`, then `defer l.Release()`, then `f()`.
