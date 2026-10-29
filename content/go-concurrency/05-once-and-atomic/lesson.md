---
title: sync.Once and sync/atomic
summary: Run initialisation exactly once, and update simple numbers without a lock using atomic operations.
minutes: 30
difficulty: hard
tags: sync, atomic, once
id: go-concurrency/once-and-atomic
mode: harness
---
Two small tools solve problems a mutex handles awkwardly.

## sync.Once

A `sync.Once` runs a function **exactly once**, no matter how many goroutines call `Do`, and every caller of `Do` waits until that one run has finished:

```go
type Config struct {
	once sync.Once
	val  string
}

func (c *Config) Get() string {
	c.once.Do(func() { c.val = loadFromDisk() }) // runs once
	return c.val // safe: Do returned, so val is set
}
```

This is lazy initialisation: pay for expensive setup only when first needed, and only once. Note that if the function panics, `Do` still counts it as done. (`sync.OnceValue` and `OnceFunc` are shorthand wrappers.)

## sync/atomic

For a single number, a lock is overkill. The `atomic` package performs indivisible operations at the hardware level. The typed values `atomic.Int64`, `atomic.Int32`, `atomic.Bool` and `atomic.Pointer[T]` are the modern API:

```go
var hits atomic.Int64
hits.Add(1)        // returns the new value
n := hits.Load()
hits.Store(0)
```

The powerful primitive is **compare-and-swap**: `CompareAndSwap(old, new)` sets the value to `new` only if it is still `old`, and reports whether it did. Wrapped in a loop it builds any lock-free update, such as "raise the maximum":

```go
for {
	cur := max.Load()
	if v <= cur || max.CompareAndSwap(cur, v) {
		return
	}
	// someone else changed it first; look again
}
```

Atomics protect one variable at a time. If two values must change together, use a mutex. And do not mix atomic and plain access to the same variable.
