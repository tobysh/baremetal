---
title: Worker Pools and Pipelines
summary: Spread work across a fixed number of goroutines, keep results in order, and chain stages with channels.
minutes: 35
difficulty: hard
tags: concurrency, channels
id: go-practice/worker-pool
mode: harness
---
You have met [goroutines and channels](/learn/go-foundations/goroutines-and-channels) and the [sync package](/learn/go-foundations/sync). Two patterns turn them into real tools.

## A worker pool

Starting one goroutine per item is fine for ten items and ruinous for ten million. A **pool** starts a fixed number of workers that all read from one channel of jobs:

```go
jobs := make(chan int)
var wg sync.WaitGroup
for w := 0; w < workers; w++ {
	wg.Add(1)
	go func() {
		defer wg.Done()
		for j := range jobs { // ends when jobs is closed
			handle(j)
		}
	}()
}
for _, j := range inputs {
	jobs <- j
}
close(jobs) // tells the workers there is no more work
wg.Wait()
```

Workers finish in any order, so results arrive jumbled. To keep input order, send the **index** with each job and have the worker write into its own slot, `out[i] = f(x)`. Different goroutines write to different slots, so there is no race, and no lock is needed.

## A pipeline

A **pipeline** is a chain of stages. Each stage is a function that receives a channel, starts a goroutine, and returns a new channel:

```go
func Square(in <-chan int) <-chan int {
	out := make(chan int)
	go func() {
		defer close(out) // the stage closes what it created
		for n := range in {
			out <- n * n
		}
	}()
	return out
}
```

`<-chan int` means "receive only", which documents (and enforces) who may send. The rule that makes pipelines work: **whoever creates a channel closes it**, and a stage closes its output when its input is closed. Then a final `for v := range ch` ends cleanly.

Forgetting to `close` is the classic bug: the reader waits forever and the program deadlocks.
