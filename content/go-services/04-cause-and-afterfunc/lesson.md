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

## A worked example

A worker pool wants to know why a shared context ended, not just that it did:

```go
ctx, cancel := context.WithCancelCause(context.Background())

go func() {
	if err := loadConfig(); err != nil {
		cancel(fmt.Errorf("config: %w", err)) // first cancel wins
	}
}()

<-ctx.Done()
fmt.Println(ctx.Err())          // context canceled
fmt.Println(context.Cause(ctx)) // config: open app.yaml: no such file
```

`AfterFunc` is the right tool when a blocking call does not take a context. Closing a connection unblocks a pending `Read`:

```go
stop := context.AfterFunc(ctx, func() { conn.Close() })
defer stop()
n, err := conn.Read(buf) // returns an error once ctx ends
```

## Pitfalls

- `cancel(nil)` records `context.Canceled` as the cause, so a nil is not "no reason".
- Always call `cancel` eventually, even `cancel(nil)`, or the context's resources leak until the parent ends.
- `f` runs in its own goroutine, so it must be safe to run concurrently with the code that registered it. Do not assume it has finished when `ctx.Done()` closes.
- If `stop()` returns `false`, `f` is running or has run. Wait for it with your own channel or `sync.WaitGroup` before you reuse what it touches.

## When to use it

Use causes when several different failures can end the same context and the caller must tell them apart in logs or error messages. Use `AfterFunc` for cleanup that cannot select on `ctx.Done()`.
