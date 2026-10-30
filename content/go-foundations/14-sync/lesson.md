---
title: WaitGroup and Mutex
summary: Wait for goroutines with sync.WaitGroup and protect shared data from races with sync.Mutex.
minutes: 25
difficulty: hard
tags: concurrency, sync
id: go-foundations/sync
mode: harness
---
Channels are not the only tool. When goroutines share a variable or a map, or when you just need to wait for them to finish, you use the `sync` package.

## WaitGroup: wait for a group of goroutines

```go
var wg sync.WaitGroup
for i := 0; i < 3; i++ {
	wg.Add(1)               // before starting the goroutine, never inside it
	go func() {
		defer wg.Done()     // always runs, even on an early return
		work(i)
	}()
}
wg.Wait()                   // blocks until the counter returns to zero
```

`Add` increments a counter, `Done` decrements it, `Wait` blocks until zero. A `WaitGroup` must not be copied after first use: pass `*sync.WaitGroup` if a function needs it.

(Since Go 1.22 each iteration has its own `i`, so the closure above is safe. In older code you would see `go func(i int) {...}(i)`.)

## Data races

Two goroutines touching the same variable, with at least one writing, is a **data race**. The result is undefined: lost updates (`counter++` is read, add, write), corrupted maps (a concurrent map write is a fatal error in Go), or worse.

```go
counter := 0
for i := 0; i < 1000; i++ {
	go func() { counter++ }()   // BUG: racy
}
```

## Mutex: one at a time

A `sync.Mutex` guards a critical section. Embed it next to the data it protects:

```go
type SafeCounter struct {
	mu sync.Mutex
	m  map[string]int
}

func (c *SafeCounter) Inc(key string) {
	c.mu.Lock()
	defer c.mu.Unlock()     // released on every path out
	c.m[key]++
}
```

Keep the locked section short and never copy a struct containing a mutex (use pointer receivers). A `sync.RWMutex` allows many readers *or* one writer, useful when reads dominate.

## Which to use?

Use a channel to **transfer ownership** of data or to coordinate stages. Use a mutex to **protect state** that several goroutines read and write, like a cache or counters. Use a WaitGroup to **wait**.
