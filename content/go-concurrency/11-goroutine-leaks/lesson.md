---
title: Goroutine Leaks
summary: Spot goroutines that can never finish, and fix them with buffered results and done channels.
minutes: 35
difficulty: hard
tags: goroutines, leaks, shutdown
id: go-concurrency/goroutine-leaks
mode: harness
---
A goroutine is cheap, but not free, and the runtime never kills one for you. A goroutine that is blocked forever is a **leak**: it holds its stack and everything it references until the program exits. Services that leak a few goroutines per request run out of memory after days.

Almost every leak is a goroutine **blocked on a channel nobody will ever use again**.

## Leak 1: the abandoned sender

```go
func First(fns ...func() int) int {
	ch := make(chan int) // unbuffered
	for _, f := range fns {
		go func() { ch <- f() }()
	}
	return <-ch // takes ONE result and returns
}
```

The winner's send is received. Every other goroutine finishes its work and then blocks on `ch <- ...` forever: nobody will receive again. The fix is to make sure the send can always complete. A channel with room for every result does that:

```go
ch := make(chan int, len(fns)) // every sender has a slot; none can block
```

## Leak 2: the abandoned producer

```go
func Counter() <-chan int {
	ch := make(chan int)
	go func() {
		for i := 0; ; i++ {
			ch <- i // blocks forever once the reader stops
		}
	}()
	return ch
}
```

The consumer may stop reading at any time (it found what it wanted, hit an error, or its request was cancelled). The producer needs a way to learn that. A **done channel** closed by the consumer is the classic signal, and `ctx.Done()` is its modern form:

```go
select {
case ch <- i:
case <-done:
	return
}
```

## Finding leaks

`runtime.NumGoroutine()` returns how many goroutines exist. A rising count over time is the telltale sign. The tests here measure it before and after your code runs, and give goroutines a moment to exit. For real programs, `pprof`'s goroutine profile shows exactly where each one is stuck.

The rule of thumb: **before you write `go`, know how that goroutine will end.**
