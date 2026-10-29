---
title: Deadlines and Timeouts
summary: Bound how long work may take with WithTimeout and WithDeadline, and read the time budget back out of a context.
minutes: 30
difficulty: medium
tags: context, timeouts
id: go-services/deadlines
mode: harness
---
A request that takes too long is often as bad as one that fails. Contexts carry a **deadline**: a point in time after which `ctx.Done()` closes by itself.

```go
ctx, cancel := context.WithTimeout(parent, 2*time.Second)
defer cancel() // releases the timer early if we finish first

res, err := fetch(ctx)
if errors.Is(err, context.DeadlineExceeded) {
	// we ran out of time
}
```
`WithDeadline(parent, t)` is the same with an absolute time. If the parent already has an earlier deadline, the child keeps the **earlier** one: a child can shorten a budget but never extend it.

## Reading the budget

```go
if dl, ok := ctx.Deadline(); ok {
	left := time.Until(dl)
}
```
`ok` is false when there is no deadline.

## Code that ignores the context

A well-behaved function watches `ctx.Done()`. A function you do not control might not. You can still stop *waiting* for it:

```go
done := make(chan result, 1) // buffered: the goroutine can always finish its send
go func() { done <- slow() }()
select {
case r := <-done:
	return r
case <-ctx.Done():
	return ctx.Err()
}
```
The buffer matters. With an unbuffered channel the abandoned goroutine would block on its send forever and leak.
