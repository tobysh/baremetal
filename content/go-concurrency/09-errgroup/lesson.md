---
title: Build an errgroup
summary: Run goroutines that can fail, return the first error, and cancel the rest, using only the standard library.
minutes: 40
difficulty: hard
tags: errors, context, sync
id: go-concurrency/errgroup
mode: harness
---
A `WaitGroup` waits, but it knows nothing about errors. Real concurrent code usually wants three things: run several tasks in parallel, get the **first error**, and **stop the others** as soon as one fails. The `golang.org/x/sync/errgroup` package does exactly that. It is not in the standard library, but it is small enough to write yourself, and doing so ties together three earlier lessons: [WaitGroup](/learn/go-concurrency/goroutines-and-waitgroup), [Once](/learn/go-concurrency/once-and-atomic) and [context](/learn/go-concurrency/context).

## The shape

```go
g, ctx := WithContext(parent)
for _, url := range urls {
	g.Go(func() error {
		return fetch(ctx, url) // should stop early if ctx is cancelled
	})
}
if err := g.Wait(); err != nil {
	// err is the FIRST error any task returned
}
```

## How it works

- `WithContext` derives a cancellable context from the parent and stores the `cancel` function in the group.
- `Go` is a `WaitGroup.Add(1)` plus a goroutine that calls the function and, if it returned an error, records it **once** and cancels the context.
- `Wait` waits for **all** tasks (even after a failure; they should wind down quickly because their context is cancelled), then cancels the context to release its resources, and returns the recorded error.

"Record once" means: only the first failure counts. A `sync.Once` is the right tool, so a late error (often just `context.Canceled` from a task that was told to stop) never overwrites the real cause.

Remember that cancellation is cooperative. The group cancels the context, but each task must watch it.
