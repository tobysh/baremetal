---
title: "Review: Job Runner"
summary: A consolidation exercise. Combine a semaphore, a WaitGroup, context cancellation and first-error handling in one small job runner. No new ideas, just practice.
minutes: 30
difficulty: medium
kind: review
tags: context, semaphore, errors
id: go-concurrency/review-job-runner
mode: harness
---
Time to put the course together. You will write one function that uses ideas you already know:

- a [semaphore](/learn/go-concurrency/semaphore) to cap how many jobs run at once,
- a [WaitGroup](/learn/go-concurrency/goroutines-and-waitgroup) to wait for them,
- a [context](/learn/go-concurrency/context) that is cancelled when one job fails (like the [errgroup](/learn/go-concurrency/errgroup) you built),
- a [mutex or `sync.Once`](/learn/go-concurrency/once-and-atomic) to record the first error safely,
- and writing into separate slice slots, so results stay in job order.

The small twist: jobs that have not started yet when the context is cancelled are skipped, not run.

A reasonable shape:

```go
sem := make(chan struct{}, limit)
for i, job := range jobs {
	// 1. take a slot (or stop if the context ended)
	// 2. if ctx.Err() != nil, release and stop
	// 3. wg.Add(1); go func() { defer wg.Done(); defer release; run job i }()
}
wg.Wait()
```

Check `ctx.Err()` **after** you get a slot: a job may have failed while you waited for it.
