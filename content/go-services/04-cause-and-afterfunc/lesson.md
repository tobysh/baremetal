---
title: Cancel Causes and AfterFunc
summary: Record why a context was cancelled with WithCancelCause, and run a callback when it is, with context.AfterFunc.
minutes: 30
difficulty: hard
tags: context, errors
id: go-services/cause-and-afterfunc
mode: harness
---
`ctx.Err()` only says *that* a context ended: `Canceled` or `DeadlineExceeded`. Often you want to know **why**.

## Causes

```go
ctx, cancel := context.WithCancelCause(parent)
cancel(errors.New("disk full"))

ctx.Err()            // context.Canceled
context.Cause(ctx)   // disk full
```
`context.Cause` returns the error given to the first `cancel(err)`. If the context ended some other way it falls back to `ctx.Err()`, and it returns `nil` while the context is still live. Only the first cancel counts; later calls change nothing.

`context.WithTimeoutCause(parent, d, cause)` does the same for deadlines, so a timeout can say *which* operation timed out instead of a bare `context deadline exceeded`.

## AfterFunc

```go
stop := context.AfterFunc(ctx, func() { cleanup() })
```
When `ctx` ends, `f` runs once in its **own goroutine**. `stop()` unregisters it: it returns `true` if it stopped `f` from ever running, and `false` if `f` has already started (or ran). This replaces the old "goroutine that waits on `<-ctx.Done()`" for cleanup such as closing a connection to unblock a blocked read.
