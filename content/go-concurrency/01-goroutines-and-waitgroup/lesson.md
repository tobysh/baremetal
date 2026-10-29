---
title: Goroutines and WaitGroup
summary: Start many goroutines, wait for all of them, and avoid the classic loop-variable and Add-after-go mistakes.
minutes: 25
difficulty: medium
tags: goroutines, sync
id: go-concurrency/goroutines-and-waitgroup
mode: harness
---
You have met [goroutines](/learn/go-foundations/goroutines-and-channels) and the [sync package](/learn/go-foundations/sync). This course builds on the [worker pool](/learn/go-practice/worker-pool) lesson and goes much deeper. First, the foundation: starting goroutines and knowing when they are done.

`go f()` starts `f` in a new goroutine and returns immediately. If `main` (or your function) returns first, the goroutine is simply abandoned. A `sync.WaitGroup` is a counter that lets you wait:

```go
var wg sync.WaitGroup
for _, t := range tasks {
	wg.Add(1) // BEFORE the go statement
	go func() {
		defer wg.Done() // always runs, even on panic
		t()
	}()
}
wg.Wait() // blocks until the counter is back to zero
```

Three rules worth memorising:

1. **Call `Add` before `go`**, in the goroutine that will call `Wait`. If the new goroutine called `Add` itself, `Wait` might run first and return too early.
2. **`defer wg.Done()`** so a panic or early return cannot leave `Wait` hanging forever.
3. **Never copy a WaitGroup**; pass a pointer if a helper needs it.

Since Go 1.22 each loop iteration has its own copy of the loop variable, so `go func() { use(t) }()` is safe. In older code you will see `t := t` or an argument passed to the closure for the same reason.

## Writing to separate slots

Many goroutines may write to *different* elements of one slice without a lock. They must never write to the same element (or append to the same slice) without synchronisation.

```go
out := make([]int, n)
for i := range out {
	wg.Add(1)
	go func() { defer wg.Done(); out[i] = compute(i) }()
}
wg.Wait() // after this, reading out is safe
```
