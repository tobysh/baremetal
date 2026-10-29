---
title: Fan-Out and Fan-In
summary: Spread one stream across several workers, then merge many streams back into one that closes at the right time.
minutes: 35
difficulty: hard
tags: channels, patterns
id: go-concurrency/fan-out-fan-in
mode: harness
---
The [worker pool](/learn/go-practice/worker-pool) lesson used an indexed slice for the results. Streams offer another shape built from two moves.

## Fan-out

Several goroutines read from the **same** channel. Go hands each value to exactly one of them, so the work is shared with no extra code:

```go
for w := 0; w < workers; w++ {
	go func() {
		for v := range in { // competing receivers
			process(v)
		}
	}()
}
```

## Fan-in

Many channels are merged into one. Start one forwarding goroutine per input, and use a `WaitGroup` to close the output **after the last forwarder has finished**:

```go
func Merge(cs ...<-chan int) <-chan int {
	out := make(chan int)
	var wg sync.WaitGroup
	wg.Add(len(cs))
	for _, c := range cs {
		go func() {
			defer wg.Done()
			for v := range c {
				out <- v
			}
		}()
	}
	go func() {
		wg.Wait()
		close(out) // only after every input is drained
	}()
	return out
}
```

Closing `out` is the subtle part. You cannot close it from a forwarder (the others may still send, and a send on a closed channel panics), and you cannot close it from the function itself (it returns immediately). A dedicated "closer" goroutine that waits and then closes is the standard answer.

Merging does not preserve ordering **between** inputs, but the order within each input is kept. Tests of fan-in code therefore sort the results or compare as sets.
