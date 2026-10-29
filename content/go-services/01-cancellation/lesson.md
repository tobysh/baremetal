---
title: Cancelling Work with Context
summary: Stop goroutines cleanly with context.WithCancel, and make every wait respond to ctx.Done().
minutes: 30
difficulty: medium
tags: context, cancellation, channels
id: go-services/cancellation
mode: harness
---
A goroutine that nobody can stop is a leak. A `context.Context` is the standard way to tell work "stop, nobody needs the answer any more".

```go
ctx, cancel := context.WithCancel(context.Background())
defer cancel() // always call it, even on the happy path

go worker(ctx)
// ... later
cancel() // ctx.Done() is closed for everyone listening
```

Two things to know:

- `ctx.Done()` is a channel that is **closed** on cancellation, so it can sit in a `select` next to other channel operations.
- `ctx.Err()` is `nil` while the context is live, then `context.Canceled` (or `context.DeadlineExceeded`, next lesson).

The pattern for a loop that must be stoppable:

```go
for i := 0; ; i++ {
	select {
	case out <- i:
	case <-ctx.Done():
		return
	}
}
```
A plain `out <- i` would block forever if the reader has gone away. Putting it in a `select` with `ctx.Done()` gives the goroutine a way out.

Waiting needs the same care. `time.Sleep` cannot be interrupted, but a timer in a `select` can:

```go
t := time.NewTimer(d)
defer t.Stop()
select {
case <-t.C:
case <-ctx.Done():
}
```
