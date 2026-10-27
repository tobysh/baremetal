---
title: Goroutines and channels
summary: Run functions concurrently with goroutines and pass data between them over channels to build a pipeline.
minutes: 25
difficulty: hard
tags: concurrency, goroutines, channels
id: go-foundations/goroutines-and-channels
mode: harness
---
Go's headline feature is cheap concurrency. A **goroutine** is a function running independently, started with the `go` keyword. It costs a few kilobytes, so programs run thousands of them.

```go
go fmt.Println("runs concurrently")
```

When `main` returns the program ends, even if goroutines are still running, and you can't know the order they run in. To coordinate you need to communicate, and Go's slogan is *do not communicate by sharing memory; share memory by communicating*.

## Channels

A **channel** is a typed pipe between goroutines:

```go
ch := make(chan int)    // unbuffered: send and receive meet
go func() { ch <- 42 }()
v := <-ch               // blocks until a value arrives
```

- An unbuffered send blocks until someone receives, and vice versa. `make(chan int, 3)` is **buffered**: sends only block when it is full.
- The sender calls `close(ch)` when there is nothing more to send. Receivers then get the remaining values, followed by zero values; `v, ok := <-ch` has `ok == false` once drained.
- `for v := range ch { ... }` receives until the channel is closed. If nobody ever closes it, the loop blocks forever.
- Only the **sender** should close, never twice, never send afterwards (panic).
- If every goroutine is blocked, the runtime stops with `fatal error: all goroutines are asleep - deadlock!`.

## Direction types and pipelines

A parameter `<-chan int` can only receive, `chan<- int` only send. A **pipeline** is a chain of stages, each a goroutine that reads from one channel and writes to the next:

```go
func double(in <-chan int) <-chan int {
	out := make(chan int)
	go func() {
		defer close(out)          // always close what you create and write to
		for v := range in {
			out <- v * 2
		}
	}()
	return out
}
```

The stage creates its output channel, starts a goroutine to fill it, and returns immediately, so stages run concurrently and values flow through as soon as they are ready.

## select

`select` waits on several channel operations and runs the one that is ready first. It is also how you add a timeout: `case <-time.After(time.Second):`.
