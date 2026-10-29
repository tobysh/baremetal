package main

import (
	"fmt"
	"sync"
	"time"
)

type Clock interface {
	Now() time.Time
}

type SystemClock struct{}

func (SystemClock) Now() time.Time { return time.Now() }

type FakeClock struct {
	mu  sync.Mutex
	now time.Time
}

func NewFakeClock(start time.Time) *FakeClock {
	return &FakeClock{now: start}
}

func (c *FakeClock) Now() time.Time {
	c.mu.Lock()
	defer c.mu.Unlock()
	return c.now
}

func (c *FakeClock) Advance(d time.Duration) {
	c.mu.Lock()
	c.now = c.now.Add(d)
	c.mu.Unlock()
}

type entry struct {
	value   string
	expires time.Time
}

type TTLCache struct {
	clock Clock
	ttl   time.Duration
	mu    sync.Mutex
	m     map[string]entry
}

func NewTTLCache(clock Clock, ttl time.Duration) *TTLCache {
	if clock == nil {
		clock = SystemClock{}
	}
	return &TTLCache{clock: clock, ttl: ttl, m: map[string]entry{}}
}

func (c *TTLCache) Set(key, value string) {
	c.mu.Lock()
	defer c.mu.Unlock()
	c.m[key] = entry{value, c.clock.Now().Add(c.ttl)}
}

func (c *TTLCache) expired(e entry) bool {
	return !c.clock.Now().Before(e.expires)
}

func (c *TTLCache) Get(key string) (string, bool) {
	c.mu.Lock()
	defer c.mu.Unlock()
	e, ok := c.m[key]
	if !ok {
		return "", false
	}
	if c.expired(e) {
		delete(c.m, key)
		return "", false
	}
	return e.value, true
}

func (c *TTLCache) Len() int {
	c.mu.Lock()
	defer c.mu.Unlock()
	for k, e := range c.m {
		if c.expired(e) {
			delete(c.m, k)
		}
	}
	return len(c.m)
}

func main() {
	clock := NewFakeClock(time.Date(2026, 1, 1, 0, 0, 0, 0, time.UTC))
	cache := NewTTLCache(clock, time.Minute)
	cache.Set("a", "1")
	clock.Advance(59 * time.Second)
	fmt.Println(cache.Get("a"))
	clock.Advance(time.Second)
	fmt.Println(cache.Get("a"))
}
