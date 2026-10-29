---
title: Context Cancellation and Deadlines
summary: Tell goroutines to stop with context, honour ctx.Done in every blocking operation, and add timeouts.
minutes: 35
difficulty: hard
tags: context, cancellation
id: go-concurrency/context
mode: harness
---
A goroutine cannot be killed from outside. Go's answer is cooperation: you hand work a `context.Context`, and when you are done with it, you cancel the context. The work notices and returns.

```go
ctx, cancel := context.WithCancel(context.Background())
defer cancel() // always call cancel, or the context's resources leak

go worker(ctx)
// ...
cancel() // asks worker to stop
```

## What a context offers

- `ctx.Done()` is a channel that is **closed** when the context is cancelled or its deadline passes. Select on it.
- `ctx.Err()` is `nil` while alive, then `context.Canceled` or `context.DeadlineExceeded`. Compare with `errors.Is`.
- `context.WithTimeout(parent, d)` and `WithDeadline(parent, t)` cancel automatically. Cancelling a parent cancels every child.

By convention a function that can block takes `ctx context.Context` as its **first** parameter. Never store a context in a struct; pass it down.

## Honour it in every blocking spot

```go
func Ticks(ctx context.Context) <-chan int {
	out := make(chan int)
	go func() {
		defer close(out)
		for i := 0; ; i++ {
			select {
			case out <- i:
			case <-ctx.Done(): // the receiver may be gone; do not block forever
				return
			}
		}
	}()
	return out
}
```

A bare `out <- i` would block forever once the reader walked away, leaking the goroutine. Every send, receive and sleep in a cancellable goroutine should sit in a `select` with `ctx.Done()`. To sleep politely use `select` with `time.After` (or a timer) rather than `time.Sleep`.
