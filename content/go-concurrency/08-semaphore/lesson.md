---
title: A Semaphore from a Channel
summary: Bound how many goroutines run a section at once with a buffered channel, with try and cancellable acquire.
minutes: 30
difficulty: hard
tags: channels, semaphore, limits
id: go-concurrency/semaphore
mode: harness
---
A **semaphore** limits how many goroutines may be inside a section at once: database connections, open files, outgoing requests. Go has no semaphore type in the standard library, but a buffered channel is one:

```go
sem := make(chan struct{}, 3) // capacity = the limit

sem <- struct{}{}        // acquire: blocks while 3 are already held
defer func() { <-sem }() // release: frees a slot
```

The buffer is the pool of permits. A send **takes** a slot, and blocks when the buffer is full; a receive **returns** it. `struct{}` carries no data, so it costs nothing.

## Try and cancel

Because acquiring is a channel send, `select` gives you the variants for free:

```go
select {
case sem <- struct{}{}:
	return true // got one
default:
	return false // all busy; do something else
}
```

```go
select {
case sem <- struct{}{}:
	return nil
case <-ctx.Done():
	return ctx.Err() // gave up waiting; no slot was taken
}
```

## Why not a pool?

Unlike the worker pool, a semaphore does not fix the number of goroutines. You can start one goroutine per item and let the semaphore decide how many actually work. That is convenient when each item is already its own call (e.g. one HTTP handler per request).

Always release what you acquire, usually with `defer`. Releasing without having acquired would block forever on the empty channel, or steal a slot.
