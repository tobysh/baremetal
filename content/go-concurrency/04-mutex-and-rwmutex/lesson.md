---
title: Mutex and RWMutex
summary: Protect shared state with a lock, keep the critical section small, and use a read-write lock for read-heavy maps.
minutes: 30
difficulty: medium
tags: sync, mutex, data-race
id: go-concurrency/mutex-and-rwmutex
mode: harness
---
Channels pass ownership of data. Sometimes it is simpler to share a variable and guard it with a lock. A **data race** is two goroutines touching the same memory at once with at least one write and no synchronisation. The result is undefined: lost updates, torn values, or (for maps) a fatal `concurrent map writes` crash.

## sync.Mutex

```go
type Counter struct {
	mu sync.Mutex
	n  int
}

func (c *Counter) Inc() {
	c.mu.Lock()
	defer c.mu.Unlock()
	c.n++
}
```

Habits that keep locks safe:

- Keep the mutex **next to the data it guards**, and say so in a comment.
- Lock and `defer Unlock()` on the next line, so no return path forgets it.
- Keep the critical section short, and never call unknown code (callbacks, channel sends) while holding the lock.
- Use pointer receivers. Copying a struct copies the mutex, and the copy no longer guards the original.
- A `Mutex` is **not re-entrant**: locking it twice from one goroutine deadlocks. If method `A` locks and calls method `B`, make `B` an unlocked helper.

## sync.RWMutex

When reads vastly outnumber writes, many readers can safely look at once. `RLock`/`RUnlock` admit any number of readers; `Lock` waits for them all and then excludes everyone.

```go
func (c *Cache) Get(k string) (int, bool) {
	c.mu.RLock()
	defer c.mu.RUnlock()
	v, ok := c.m[k]
	return v, ok
}
```

Never write while holding only a read lock. And a read-modify-write such as "increment the value for k" must be **one** `Lock` section, not an `RLock` read followed by a separate `Lock` write: another goroutine can slip in between.

Run your tests with `go test -race` on your own machine to catch races early; here the tests simply hammer your code from many goroutines.
